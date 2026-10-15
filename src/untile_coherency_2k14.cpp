// GPU->CPU coherency for render-to-texture data the game's CPU code reads (GitHub issue #1).
//
// WWE 2K14 composites some wrestler textures on the GPU: managers such as Miss Elizabeth and
// Stephanie McMahon, alternate and created attires (e.g. an alternate John Cena shirt), retro
// models. It renders a 2048x1024 layer plus smaller mip copies, resolves them into guest memory,
// then untiles/converts them on the CPU with XGUntileSurface. The recompiled GPU keeps resolve
// results in its own buffer, so without a readback the CPU untiles stale memory and the model
// renders black or rainbow-striped. Reading back every resolve (readback_resolve=full) fixes it
// but halves the in-ring frame rate, and limiting it to an address range is unreliable because
// the composite buffers move between matches. This hook reads back exactly the source surface of
// each untile, wherever it is, and only when the game untiles (at load / entrance time).
//
// Port of WWE '13 Recomp's resolve_coherency.cpp (XGUntileSurface sub_82BE19E8 there).
//
// XGUntileSurface (TU 2.0.1.0 image): sub_82CFD800(dest=r3, row_pitch=r4, point=r5, source=r6,
// width=r7, height=r8, rect=r9, texel_pitch=r10). Identified by the XDK body: a NULL rect becomes
// {0,0,width,height}, a NULL point becomes {0,0}, and for an in-place untile (source == dest) it
// allocates align32(width) * align32(height) * texel_pitch bytes (rounded to 4 KB) and copies the
// tiled source there first. That size is the tiled source extent read back below.
//
// Kill switch: WWE2K14_UNTILE_COHERENCY=0.

#include <atomic>
#include <cstdint>
#include <cstdlib>

#include <rex/graphics/command_processor.h>
#include <rex/graphics/graphics_system.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/memory.h>
#include <rex/runtime.h>
#include <rex/system/kernel_state.h>

extern "C" PPC_FUNC(__imp__sub_82CFD800);  // original XGUntileSurface

namespace {

bool UntileCoherencyEnabled() {
  static const bool enabled = [] {
    const char* v = std::getenv("WWE2K14_UNTILE_COHERENCY");
    return !(v && v[0] == '0');
  }();
  return enabled;
}

void MakeUntileSourceVisible(uint32_t source, uint32_t width, uint32_t height,
                             uint32_t texel_pitch) {
  const uint64_t size =
      uint64_t((width + 31) & ~31u) * ((height + 31) & ~31u) * uint64_t(texel_pitch);
  if (!source || !size || size > 64u * 1024 * 1024) {
    return;
  }
  auto* kernel_state = REX_KERNEL_STATE();
  auto* graphics = kernel_state ? static_cast<rex::graphics::GraphicsSystem*>(
                                      kernel_state->emulator()->graphics_system())
                                : nullptr;
  if (!graphics || !graphics->command_processor()) {
    return;
  }
  const bool done =
      graphics->command_processor()->MakeGpuWritesVisibleToCpu(source, uint32_t(size));
  static std::atomic<uint32_t> logged{0};
  if (logged.fetch_add(1, std::memory_order_relaxed) < 16) {
    REXLOG_INFO("[wwe2k14-untile] source={:08X} {}x{} texel_pitch={} bytes={} readback={}", source,
                width, height, texel_pitch, size, done ? "yes" : "no");
  }
}

}  // namespace

extern "C" PPC_FUNC(sub_82CFD800) {
  if (UntileCoherencyEnabled()) {
    MakeUntileSourceVisible(ctx.r6.u32, ctx.r7.u32, ctx.r8.u32, ctx.r10.u32);
  }
  __imp__sub_82CFD800(ctx, base);
}
