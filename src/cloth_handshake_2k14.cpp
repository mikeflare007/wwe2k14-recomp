// WWE 2K14 Havok-cloth handshake race fix (port of WWE '13's cloth_handshake.cpp, which was itself
// a port of the original 2K14 fix for the Undertaker-entrance freeze).
//
// The HavokClothThread waits on the manual-reset "StartCloth" event (obj+0x68), runs one cloth job
// and sets "EndCloth" (obj+0x6C). The game's begin() sub_8251E6D0 does:
//   [obj+0x64] = 1; SetEvent(StartCloth); ClearEvent(EndCloth);
// On the 360 the worker can't finish between those two calls; on PC it runs on another core and can
// set EndCloth before begin() clears it, wiping the completion signal (end() then waits forever, or the
// frame uses a cloth result that was never handed over). Fix: clear EndCloth first, then signal.
//
// Addresses (TU 2.0.1.0 image): singleton 0x83763088 (stored by the constructor sub_8251EC10 at
// 0x8251ECE0); event names "StartCloth" 0x8201C698 / "EndCloth" 0x8201C68C; set-event wrapper
// sub_822622E0, clear-event wrapper sub_82C929E0 (both called by the original begin()).

#include <atomic>
#include <cstdint>

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/memory.h>

namespace {
constexpr uint32_t kClothSingleton = 0x83763088;
constexpr uint32_t kRunFlag = 0x64;
constexpr uint32_t kStartEvent = 0x68;
constexpr uint32_t kEndEvent = 0x6C;
}  // namespace

extern "C" PPC_FUNC(__imp__sub_822622E0);  // set-event wrapper
extern "C" PPC_FUNC(__imp__sub_82C929E0);  // clear-event wrapper

extern "C" PPC_FUNC(sub_8251E6D0) {
  const uint32_t object = PPC_LOAD_U32(kClothSingleton);
  if (object == 0) {
    return;
  }
  const uint32_t start_event = PPC_LOAD_U32(object + kStartEvent);
  if (start_event == 0) {
    return;
  }
  PPC_STORE_U32(object + kRunFlag, 1);

  // Clear the previous completion signal first, then ask the worker to run.
  ctx.r3.u32 = PPC_LOAD_U32(object + kEndEvent);
  ctx.lr = 0x8251E710;
  __imp__sub_82C929E0(ctx, base);
  ctx.r3.u32 = PPC_LOAD_U32(object + kStartEvent);
  ctx.lr = 0x8251E708;
  __imp__sub_822622E0(ctx, base);

  static std::atomic<bool> logged{false};
  if (!logged.exchange(true, std::memory_order_relaxed)) {
    REXLOG_INFO("[wwe2k14-cloth] handshake reorder active: ClearEvent(EndCloth) before SetEvent(StartCloth)");
  }
}
