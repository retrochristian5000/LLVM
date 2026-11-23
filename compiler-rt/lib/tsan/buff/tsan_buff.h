//===-- tsan_buff.h --------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Utilities for buffers of program-counter entries. Shadow-stack storage
// remains in tsan_stack.cpp; stack unwinding remains in tsan_stack_trace.cpp.

#ifndef TSAN_BUFF_H
#define TSAN_BUFF_H

#include "../rtl/tsan_defs.h"

namespace __tsan {

// Reverse a buffer of count program counters in place. A null buffer is
// permitted only when count is zero.
void ReversePcBuffer(uptr *pcs, uptr count);

}  // namespace __tsan

#endif  // TSAN_BUFF_H
