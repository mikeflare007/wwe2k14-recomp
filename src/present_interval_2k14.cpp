// WWE 2K14: 60 FPS everywhere (entrances, cutscenes and matches normally ask for 30).
//
// Same mechanism as WWE '13's present_interval.cpp: every D3D present-interval change goes through the
// one-instruction setter sub_82CAE118 (`stw r4,14000(r3)`: device +0x36B0, read by the swap path; found by
// pattern in the TU 2.0.1.0 image). Replacing interval 2+ with 1 presents every vblank -> 60 FPS.
// On by default; WWE2K14_KEEP_30=1 (or WWE13_KEEP_30=1 from the auto-tester) keeps the game's own 30 FPS.
// The strong definition overrides the generated weak alias of __imp__sub_82CAE118.

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/memory.h>

namespace {
bool Keep30() {
  static const bool keep = [] {
    for (const char* name : {"WWE2K14_KEEP_30", "WWE13_KEEP_30"}) {
      const char* v = std::getenv(name);
      if (v != nullptr && std::strcmp(v, "1") == 0) {
        return true;
      }
    }
    return false;
  }();
  return keep;
}
}  // namespace

extern "C" PPC_FUNC(sub_82CAE118) {
  const uint32_t requested = ctx.r4.u32;
  uint32_t interval = requested;
  if (interval > 1 && !Keep30()) {
    interval = 1;
  }
  static uint32_t last_requested = UINT32_MAX;
  if (requested != last_requested) {
    last_requested = requested;
    REXLOG_INFO("[wwe2k14-interval] game requested {} -> set {}", requested, interval);
  }
  PPC_STORE_U32(ctx.r3.u32 + 14000, interval);
}
