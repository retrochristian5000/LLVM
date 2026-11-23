//===-- tsan_stack.h -------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Thread shadow-stack storage and stack depot integration.
// Stack-trace buffers and unwinding remain in tsan_stack_trace.h.

#ifndef TSAN_STACK_H
#define TSAN_STACK_H

#include "../rtl/tsan_defs.h"

namespace __tsan {

void InitializeShadowStack(ThreadState *thr);
void DestroyShadowStack(ThreadState *thr);

#if SANITIZER_APPLE && !SANITIZER_GO
// Record a newly started Darwin thread's stack as initialized. ThreadStart
// handles the main-thread exception and TLS initialization separately.
void ImitateDarwinStackInitialization(ThreadState *thr);
#endif

#if SANITIZER_OSX && !SANITIZER_GO
// Use a worker thread's cached pthread stack only when the frame and active
// execution stack are inside it. This helper also supports bounds testing.
bool TryMacCachedStackBounds(uptr cached_begin, uptr cached_size,
                             uptr frame_pointer, uptr current_sp,
                             uptr *stack_top, uptr *stack_bottom);
// Fallback to sanitizer_common when the cached range is not applicable.
void GetMacStackBounds(uptr frame_pointer, uptr *stack_top,
                       uptr *stack_bottom);
#endif

StackID CurrentStackId(ThreadState *thr, uptr pc);

#if SANITIZER_GO
void GrowShadowStack(ThreadState *thr);
#endif

}  // namespace __tsan

#endif  // TSAN_STACK_H
