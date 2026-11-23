//===-- tsan_ignore.cpp ---------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Runtime ignore scopes used by TSan annotations and interceptors. Compiler
// pragmas are handled by Clang, not by this module.

#include "tsan_ignore.h"

#include "tsan_rtl.h"

namespace __tsan {

// The counters are thread-local: no atomic operation or extra memory
// barrier is needed for scope nesting on the calling thread.
static void UpdateIgnoreDepth(int *depth, bool entering) {
#if defined(__aarch64__) && !SANITIZER_GO
  static_assert(sizeof(int) == sizeof(u32),
                "AArch64 ignore depth requires a 32-bit int");
  // Invalid transitions leave the counter unmodified.
  CHECK(__tsan_ignore_aarch64_depth_step(depth, entering ? 1 : -1));
#else
  if (entering) {
    CHECK_GE(*depth, 0);
    CHECK_LT(*depth, 0x7fffffff);
    ++*depth;
  } else {
    CHECK_GT(*depth, 0);
    --*depth;
  }
#endif
}

void ThreadIgnoreBegin(ThreadState* thr, uptr pc) {
  DPrintf("#%d: ThreadIgnoreBegin\n", thr->tid);
  UpdateIgnoreDepth(&thr->ignore_reads_and_writes, true);
  thr->fast_state.SetIgnoreBit();
#if !SANITIZER_GO
  if (pc && !ctx->after_multithreaded_fork)
    thr->mop_ignore_set.Add(CurrentStackId(thr, pc));
#endif
}

void ThreadIgnoreEnd(ThreadState *thr) {
  DPrintf("#%d: ThreadIgnoreEnd\n", thr->tid);
  UpdateIgnoreDepth(&thr->ignore_reads_and_writes, false);
  if (thr->ignore_reads_and_writes == 0) {
    thr->fast_state.ClearIgnoreBit();
#if !SANITIZER_GO
    thr->mop_ignore_set.Reset();
#endif
  }
}

void ThreadIgnoreSyncBegin(ThreadState *thr, uptr pc) {
  DPrintf("#%d: ThreadIgnoreSyncBegin\n", thr->tid);
  UpdateIgnoreDepth(&thr->ignore_sync, true);
#if !SANITIZER_GO
  if (pc && !ctx->after_multithreaded_fork)
    thr->sync_ignore_set.Add(CurrentStackId(thr, pc));
#endif
}

void ThreadIgnoreSyncEnd(ThreadState *thr) {
  DPrintf("#%d: ThreadIgnoreSyncEnd\n", thr->tid);
  UpdateIgnoreDepth(&thr->ignore_sync, false);
#if !SANITIZER_GO
  if (thr->ignore_sync == 0)
    thr->sync_ignore_set.Reset();
#endif
}

}  // namespace __tsan
