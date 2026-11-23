//===-- tsan_stack_trace.cpp ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
//===----------------------------------------------------------------------===//
#include "tsan_stack_trace.h"
#include "tsan_stack.h"
#include "../rtl/tsan_rtl.h"


#if !SANITIZER_GO
void __sanitizer::BufferedStackTrace::UnwindImpl(
    uptr pc, uptr bp, void *context, bool request_fast, u32 max_depth) {
  uptr top = 0;
  uptr bottom = 0;
#if SANITIZER_OSX
  __tsan::GetMacStackBounds(bp, &top, &bottom);
#else
  GetThreadStackTopAndBottom(false, &top, &bottom);
#endif
  bool fast = StackTrace::WillUseFastUnwind(request_fast);
  Unwind(max_depth, pc, bp, context, top, bottom, fast);
}
#endif  // SANITIZER_GO
