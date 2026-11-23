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

#include "tsan_defs.h"

namespace __tsan {

void InitializeShadowStack(ThreadState *thr);
void DestroyShadowStack(ThreadState *thr);

StackID CurrentStackId(ThreadState *thr, uptr pc);

#if SANITIZER_GO
void GrowShadowStack(ThreadState *thr);
#endif

}  // namespace __tsan

#endif  // TSAN_STACK_H
