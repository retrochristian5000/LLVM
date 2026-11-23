//===-- tsan_stack_mac.cpp ------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// macOS stack bounds for TSan stack-trace unwinding. ThreadStart already
// caches pthread stack limits in ThreadState, so reuse them for worker-thread
// frames instead of querying pthread_get_stackaddr_np/stacksize_np again.
//
// The main thread retains the platform query: older macOS releases can
// report a different main-thread stack size during initialization.
// Contexts on another stack (including signal alternate stacks and fibers)
// also use the sanitizer_common fallback. No global cache is introduced.
//
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_OSX && !SANITIZER_GO

#include "tsan_stack.h"
#include "sanitizer_common/sanitizer_common.h"
#include "../rtl/tsan_rtl.h"

namespace __tsan {

bool TryMacCachedStackBounds(uptr cached_begin, uptr cached_size,
                             uptr frame_pointer, uptr current_sp,
                             uptr *stack_top, uptr *stack_bottom) {
  // Reject absent or wrapping stack limits without touching caller outputs.
  if (!cached_begin || !cached_size ||
      cached_size > ~static_cast<uptr>(0) - cached_begin)
    return false;

  const uptr end = cached_begin + cached_size;
  // Require both the frame being unwound and the current execution stack to
  // be inside the pthread stack. A fiber or signal alternate stack must not
  // borrow the cached bounds from its underlying worker thread.
  if (frame_pointer < cached_begin || frame_pointer >= end ||
      current_sp < cached_begin || current_sp >= end)
    return false;

  *stack_bottom = cached_begin;
  *stack_top = end;
  return true;
}

void GetMacStackBounds(uptr frame_pointer, uptr *stack_top,
                       uptr *stack_bottom) {
  ThreadState *thr = cur_thread();
  // Only worker pthread stacks use the cache. The generic OS query remains
  // authoritative for the main thread and during runtime initialization.
  if (thr && thr->is_inited && !thr->is_dead && thr->tid != kMainTid) {
    const uptr marker = 0;
    if (TryMacCachedStackBounds(thr->stk_addr, thr->stk_size, frame_pointer,
                                reinterpret_cast<uptr>(&marker), stack_top,
                                stack_bottom))
      return;
  }

  GetThreadStackTopAndBottom(false, stack_top, stack_bottom);
}

}  // namespace __tsan

#endif  // SANITIZER_OSX && !SANITIZER_GO
