//===-- tsan_pragma.cpp ---------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Runtime suppression scopes used by TSan annotations and interceptors.
// Clang's no_sanitize("thread") pragma affects compiler instrumentation;
// runtime annotations affect the current thread's ignore state separately.

#include "tsan_pragma.h"

#include "tsan_rtl.h"

namespace __tsan {

void ThreadIgnoreBegin(ThreadState* thr, uptr pc) {
  DPrintf("#%d: ThreadIgnoreBegin\n", thr->tid);
  thr->ignore_reads_and_writes++;
  CHECK_GT(thr->ignore_reads_and_writes, 0);
  thr->fast_state.SetIgnoreBit();
#if !SANITIZER_GO
  if (pc && !ctx->after_multithreaded_fork)
    thr->mop_ignore_set.Add(CurrentStackId(thr, pc));
#endif
}

void ThreadIgnoreEnd(ThreadState *thr) {
  DPrintf("#%d: ThreadIgnoreEnd\n", thr->tid);
  CHECK_GT(thr->ignore_reads_and_writes, 0);
  thr->ignore_reads_and_writes--;
  if (thr->ignore_reads_and_writes == 0) {
    thr->fast_state.ClearIgnoreBit();
#if !SANITIZER_GO
    thr->mop_ignore_set.Reset();
#endif
  }
}


void ThreadIgnoreSyncBegin(ThreadState *thr, uptr pc) {
  DPrintf("#%d: ThreadIgnoreSyncBegin\n", thr->tid);
  thr->ignore_sync++;
  CHECK_GT(thr->ignore_sync, 0);
#if !SANITIZER_GO
  if (pc && !ctx->after_multithreaded_fork)
    thr->sync_ignore_set.Add(CurrentStackId(thr, pc));
#endif
}

void ThreadIgnoreSyncEnd(ThreadState *thr) {
  DPrintf("#%d: ThreadIgnoreSyncEnd\n", thr->tid);
  CHECK_GT(thr->ignore_sync, 0);
  thr->ignore_sync--;
#if !SANITIZER_GO
  if (thr->ignore_sync == 0)
    thr->sync_ignore_set.Reset();
#endif
}

}  // namespace __tsan
