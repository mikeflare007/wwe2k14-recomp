// WWE 2K14 startup-sequence probe (diagnostic, env-gated: WWE2K14_STARTUP_PROBE=1).
//
// The game stays on the legal screen forever. Xenia Canary (same TU 2.0.1.0
// code) shows it for ~300 frames, then releases it and loads FONTHD/MENU/the
// 2K logo. The file pipeline and the main thread's per-frame event pattern are
// identical between the two, so the stuck decision is inside game logic.
//
// This wraps every function referenced by the CStartupCtrlSequence tables
// (0x82033980-0x82033B70 in the patched image) and logs: the first calls, any
// call whose (r4, return value) differs from that function's previous call,
// every 600th call, and once every 2 s a per-function call-count summary. For
// the slot-state handlers (obj + (r4 + 0x43) * 0x20 holds the slot state) it
// also logs the slot state before and after the call.
//
// Each wrapper is a strong definition of sub_XXXXXXXX; the generated function is
// a weak alias of __imp__sub_XXXXXXXX and is replaced at link time (same
// mechanism as the WWE '13 cloth_handshake.cpp override).

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/memory.h>

namespace {

bool ProbeEnabled() {
  static const bool enabled = [] {
    // WWE13_ prefix too: the auto-tester only forwards WWE13_* / REX_* variables.
    for (const char* name : {"WWE2K14_STARTUP_PROBE", "WWE13_STARTUP_PROBE"}) {
      const char* v = std::getenv(name);
      if (v != nullptr && std::strcmp(v, "1") == 0) {
        return true;
      }
    }
    return false;
  }();
  return enabled;
}

struct FnStats {
  const char* name = nullptr;
  std::atomic<uint64_t> calls{0};
  std::atomic<uint64_t> calls_at_last_summary{0};
  uint32_t last_r4 = 0xFFFFFFFF;
  uint32_t last_ret = 0xFFFFFFFF;
};

constexpr int kMaxFns = 40;
std::array<FnStats, kMaxFns> g_stats;
std::atomic<int> g_registered{0};
std::mutex g_log_mutex;

bool PlausibleGuestPtr(uint32_t p) {
  return (p >= 0x40000000u && p < 0x80000000u) || (p >= 0xA0000000u && p < 0xC0000000u);
}

void SummaryThread() {
  for (;;) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::string line;
    for (int i = 0; i < g_registered.load(); ++i) {
      auto& s = g_stats[i];
      uint64_t now = s.calls.load(std::memory_order_relaxed);
      uint64_t prev = s.calls_at_last_summary.exchange(now, std::memory_order_relaxed);
      if (now != prev) {
        line += std::string(" ") + s.name + "+" + std::to_string(now - prev);
      }
    }
    REXLOG_INFO("[startup-probe] 2s calls:{}", line.empty() ? std::string(" (none)") : line);
  }
}

void EnsureSummaryThread() {
  static std::once_flag once;
  std::call_once(once, [] {
    REXLOG_INFO("[startup-probe] active");
    std::thread(SummaryThread).detach();
  });
}

int Register(const char* name) {
  int idx = g_registered.fetch_add(1);
  if (idx >= kMaxFns) {
    return -1;
  }
  g_stats[idx].name = name;
  return idx;
}

template <int kSlotForm>
void Probe(int idx, const char* name, PPCContext& ctx, uint8_t* base,
           void (*impl)(PPCContext&, uint8_t*)) {
  if (!ProbeEnabled() || idx < 0) {
    impl(ctx, base);
    return;
  }
  EnsureSummaryThread();
  const uint32_t r3 = ctx.r3.u32, r4 = ctx.r4.u32, r5 = ctx.r5.u32, r6 = ctx.r6.u32;
  uint32_t slot_addr = 0, state_before = 0;
  if (kSlotForm && PlausibleGuestPtr(r3) && r4 < 64) {
    slot_addr = r3 + (r4 + 0x43) * 0x20;
    state_before = PPC_LOAD_U32(slot_addr);
  }
  impl(ctx, base);
  const uint32_t ret = ctx.r3.u32;
  auto& s = g_stats[idx];
  const uint64_t n = s.calls.fetch_add(1, std::memory_order_relaxed) + 1;
  uint32_t state_after = slot_addr ? PPC_LOAD_U32(slot_addr) : 0;
  bool log = false;
  {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    if (n <= 3 || r4 != s.last_r4 || ret != s.last_ret || (n % 120) == 0 ||
        state_before != state_after) {
      log = true;
    }
    s.last_r4 = r4;
    s.last_ret = ret;
  }
  if (!log) {
    return;
  }
  if (slot_addr) {
    REXLOG_INFO(
        "[startup-probe] {} n={} r3={:08X} r4={:X} r5={:X} r6={:X} -> {:X} slot[{:08X}] "
        "state {} -> {} obj1c={:08X} lr={:08X}",
        name, n, r3, r4, r5, r6, ret, slot_addr, state_before, state_after,
        PPC_LOAD_U32(slot_addr + 0x1c), (uint32_t)ctx.lr);
  } else {
    std::string extra;
    if (std::strcmp(name, "sub_827DEAB8") == 0 && PlausibleGuestPtr(r3)) {
      // Scene-sequence manager: current sequence object at +0x40, its update is vtable slot 9,
      // the current sequence index lives at 0x8292474C (table 0x82625298, 16-byte entries).
      const uint32_t seq = PPC_LOAD_U32(r3 + 0x40);
      const uint32_t idx = PPC_LOAD_U32(0x8292474C);
      uint32_t vt = 0, upd = 0, chk = 0;
      std::string words;
      if (PlausibleGuestPtr(seq)) {
        vt = PPC_LOAD_U32(seq);
        if (vt >= 0x82000000u && vt < 0x84000000u) {
          upd = PPC_LOAD_U32(vt + 36);
          chk = PPC_LOAD_U32(vt + 48);
        }
        for (uint32_t o = 0; o < 0x100; o += 4) {
          words += fmt::format("{}{:08X}", (o % 32) ? " " : (o ? " | " : ""), PPC_LOAD_U32(seq + o));
        }
      }
      extra = fmt::format(" seq={:08X} vt={:08X} upd={:08X} chk={:08X} idx={} words=[{}]", seq, vt,
                          upd, chk, idx, words);
    } else if (std::strcmp(name, "sub_827E74D0") == 0 || std::strcmp(name, "sub_827E7F90") == 0 ||
               std::strcmp(name, "sub_82804D78") == 0) {
      // Legal-screen wait: global task object at 0x83924794; busy while +8 == 1, then asks the
      // sub-task at +0x54 (vtable slot 24) whether it finished.
      const uint32_t g = PPC_LOAD_U32(0x83924794);
      std::string words;
      uint32_t sub = 0, subvt = 0, subfn = 0;
      if (PlausibleGuestPtr(g)) {
        for (uint32_t o = 0; o < 0x80; o += 4) {
          words += fmt::format("{}{:08X}", (o % 32) ? " " : (o ? " | " : ""), PPC_LOAD_U32(g + o));
        }
        sub = PPC_LOAD_U32(g + 84);
        if (PlausibleGuestPtr(sub)) {
          subvt = PPC_LOAD_U32(sub);
          if (subvt >= 0x82000000u && subvt < 0x84000000u) {
            subfn = PPC_LOAD_U32(subvt + 96);
          }
          words += " || sub:";
          for (uint32_t o = 0; o < 0x60; o += 4) {
            words += fmt::format(" {:08X}", PPC_LOAD_U32(sub + o));
          }
        }
      }
      extra = fmt::format(" G={:08X} sub={:08X} subvt={:08X} subfn24={:08X} [{}]", g, sub, subvt,
                          subfn, words);
      if (std::strcmp(name, "sub_82804D78") == 0 && PlausibleGuestPtr(r3)) {
        extra += fmt::format(" lgal+78={:X} +7C={:X} +80={:08X}(+3C={:X}) +84={:08X}(+590={:X})",
                             PPC_LOAD_U32(r3 + 0x78), PPC_LOAD_U32(r3 + 0x7C), PPC_LOAD_U32(r3 + 0x80),
                             PlausibleGuestPtr(PPC_LOAD_U32(r3 + 0x80))
                                 ? PPC_LOAD_U32(PPC_LOAD_U32(r3 + 0x80) + 0x3C) : 0,
                             PPC_LOAD_U32(r3 + 0x84),
                             PlausibleGuestPtr(PPC_LOAD_U32(r3 + 0x84))
                                 ? PPC_LOAD_U32(PPC_LOAD_U32(r3 + 0x84) + 0x590) : 0);
      }
    } else if (PlausibleGuestPtr(r3)) {
      extra = fmt::format(" f860..874={:08X} {:08X} {:08X} {:08X} {:08X} {:08X}",
                          PPC_LOAD_U32(r3 + 0x860), PPC_LOAD_U32(r3 + 0x864),
                          PPC_LOAD_U32(r3 + 0x868), PPC_LOAD_U32(r3 + 0x86c),
                          PPC_LOAD_U32(r3 + 0x870), PPC_LOAD_U32(r3 + 0x874));
    }
    REXLOG_INFO("[startup-probe] {} n={} r3={:08X} r4={:X} r5={:X} r6={:X} -> {:X} lr={:08X}{}",
                name, n, r3, r4, r5, r6, ret, (uint32_t)ctx.lr, extra);
  }
}

}  // namespace

#define WWE2K14_PROBE(addr, slot_form)                                                  \
  extern "C" PPC_FUNC(__imp__sub_##addr);                                               \
  extern "C" PPC_FUNC(sub_##addr) {                                                     \
    static const int idx = Register("sub_" #addr);                                      \
    Probe<slot_form>(idx, "sub_" #addr, ctx, base, &__imp__sub_##addr);                 \
  }

// Slot-state handlers: addi r11,r4,0x43; slwi r11,r11,5; state at obj+r11.
WWE2K14_PROBE(827E18B0, 1)
WWE2K14_PROBE(827E18E0, 1)
WWE2K14_PROBE(827E1950, 1)
WWE2K14_PROBE(827E19F0, 1)
WWE2K14_PROBE(827E1A58, 1)
WWE2K14_PROBE(827E1AC0, 1)
WWE2K14_PROBE(827E1B30, 1)
// Everything else referenced by the sequence tables.
WWE2K14_PROBE(827DEAB8, 0)
WWE2K14_PROBE(827E0F28, 0)
WWE2K14_PROBE(827E0FA0, 0)
WWE2K14_PROBE(827E0FD8, 0)
WWE2K14_PROBE(827E1030, 0)
WWE2K14_PROBE(827E1210, 0)
WWE2K14_PROBE(827E12D0, 0)
WWE2K14_PROBE(827E1440, 0)
WWE2K14_PROBE(827E1588, 0)
WWE2K14_PROBE(827E15E0, 0)
WWE2K14_PROBE(827E16D8, 0)
WWE2K14_PROBE(827E16F0, 0)
WWE2K14_PROBE(827E1B80, 0)
WWE2K14_PROBE(827E1D20, 0)
WWE2K14_PROBE(827E1E20, 0)
WWE2K14_PROBE(827E2200, 0)
WWE2K14_PROBE(827E2310, 0)
WWE2K14_PROBE(827E2430, 0)
WWE2K14_PROBE(827E2610, 0)
WWE2K14_PROBE(827E27B0, 0)
WWE2K14_PROBE(827E29E0, 0)
WWE2K14_PROBE(827E5578, 0)
WWE2K14_PROBE(827E55E0, 0)
WWE2K14_PROBE(827E58B0, 0)
WWE2K14_PROBE(827E5988, 0)
// Legal-screen ("LGAL", vtable 0x820345B8) state-1 step and the async task it waits on.
WWE2K14_PROBE(82804D78, 0)
WWE2K14_PROBE(82804E98, 0)
WWE2K14_PROBE(827E74D0, 0)
WWE2K14_PROBE(827E7F90, 0)
