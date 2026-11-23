//===-- tsan_stack.cpp ----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Thread shadow-stack allocation, growth, release, and stack depot IDs.
// Unwinding and variable-size trace buffers remain in tsan_stack_trace.cpp.

#include "tsan_stack.h"

#include "sanitizer_common/sanitizer_stackdepot.h"
#include "tsan_mman.h"
#include "tsan_rtl.h"

namespace __tsan {

void InitializeShadowStack(ThreadState *thr) {
#if !SANITIZER_GO
  // C/C++ uses a fixed-size shadow stack to avoid hot-path allocation.
  const uptr kInitStackSize = kShadowStackSize;
  thr->shadow_stack = static_cast<uptr *>(
      MmapNoReserveOrDie(kInitStackSize * sizeof(uptr), "shadow stack"));
  SetShadowRegionHugePageMode(reinterpret_cast<uptr>(thr->shadow_stack),
                              kInitStackSize * sizeof(uptr));
#else
  // Go uses a dynamically sized shadow stack.
  const uptr kInitStackSize = 8;
  thr->shadow_stack = static_cast<uptr *>(Alloc(kInitStackSize * sizeof(uptr)));
#endif
  thr->shadow_stack_pos = thr->shadow_stack;
  thr->shadow_stack_end = thr->shadow_stack + kInitStackSize;
}

void DestroyShadowStack(ThreadState *thr) {
#if !SANITIZER_GO
  UnmapOrDie(thr->shadow_stack, kShadowStackSize * sizeof(uptr));
#else
  Free(thr->shadow_stack);
#endif
  thr->shadow_stack = nullptr;
  thr->shadow_stack_pos = nullptr;
  thr->shadow_stack_end = nullptr;
}

#if SANITIZER_GO
NOINLINE
void GrowShadowStack(ThreadState *thr) {
  const int sz = thr->shadow_stack_end - thr->shadow_stack;
  const int newsz = 2 * sz;
  auto *newstack = (uptr *)Alloc(newsz * sizeof(uptr));
  internal_memcpy(newstack, thr->shadow_stack, sz * sizeof(uptr));
  Free(thr->shadow_stack);
  thr->shadow_stack = newstack;
  thr->shadow_stack_pos = newstack + sz;
  thr->shadow_stack_end = newstack + newsz;
}
#endif

StackID CurrentStackId(ThreadState *thr, uptr pc) {
#if !SANITIZER_GO
  if (!thr->is_inited)  // May happen during bootstrap.
    return kInvalidStackID;
#endif
  if (pc != 0) {
#if !SANITIZER_GO
    DCHECK_LT(thr->shadow_stack_pos, thr->shadow_stack_end);
#else
    if (thr->shadow_stack_pos == thr->shadow_stack_end)
      GrowShadowStack(thr);
#endif
    thr->shadow_stack_pos[0] = pc;
    thr->shadow_stack_pos++;
  }
  StackID id = StackDepotPut(
      StackTrace(thr->shadow_stack, thr->shadow_stack_pos - thr->shadow_stack));
  if (pc != 0)
    thr->shadow_stack_pos--;
  return id;
}

#if !SANITIZER_GO
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
uptr __tsan_testonly_shadow_stack_current_size() {
  ThreadState *thr = cur_thread();
  return thr->shadow_stack_pos - thr->shadow_stack;
}
#endif

}  // namespace __tsan
