//===-- tsan_stack_darwin.cpp ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Darwin thread-stack shadow initialization. Generic shadow-stack allocation,
// stack depot handling, and unwinding remain in tsan_stack.cpp and
// tsan_stack_trace.cpp.

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE && !SANITIZER_GO

#include "tsan_stack.h"
#include "../rtl/tsan_rtl.h"

namespace __tsan {

// Defined in tsan_thread.cpp, shared with the non-Darwin initialization path.
extern "C" void __tsan_stack_initialization();

void ImitateDarwinStackInitialization(ThreadState *thr) {
  DCHECK(thr);
  if (!thr->stk_addr || !thr->stk_size)
    return;

  const uptr pc = StackTrace::GetNextInstructionPc(
      reinterpret_cast<uptr>(__tsan_stack_initialization));
  MemoryRangeImitateWrite(thr, pc, thr->stk_addr, thr->stk_size);
}

}  // namespace __tsan

#endif  // SANITIZER_APPLE && !SANITIZER_GO
