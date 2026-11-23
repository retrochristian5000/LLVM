//===-- tsan_mman_darwin.cpp ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Darwin's allocation warning must not symbolize while holding the thread
// registry lock. Allocator implementation stays in tsan_mman.cpp.

#include "sanitizer_common/sanitizer_platform.h"
#if SANITIZER_APPLE

#include "sanitizer_common/sanitizer_placement_new.h"
#include "tsan_mman.h"
#include "tsan_report.h"
#include "tsan_rtl.h"

namespace __tsan {

void SignalUnsafeCall(ThreadState *thr, uptr pc) {
  if (atomic_load_relaxed(&thr->in_signal_handler) == 0 ||
      !ShouldReport(thr, ReportTypeSignalUnsafe))
    return;
  VarSizeStackTrace stack;
  ObtainCurrentStack(thr, pc, &stack);
  if (IsFiredSuppression(ctx, ReportTypeSignalUnsafe, stack))
    return;
  // Use alloca, because malloc during signal handling deadlocks
  ScopedReport *rep = (ScopedReport *)__builtin_alloca(sizeof(ScopedReport));
  // Take a new scope as Apple platforms require the below locks released
  // before symbolizing in order to avoid a deadlock
  {
    ThreadRegistryLock l(&ctx->thread_registry);
    new (rep) ScopedReport(ReportTypeSignalUnsafe);
    rep->AddStack(stack, true);
  }  // Release registry lock before symbolization and report output
  OutputReport(thr, *rep);

  // Need to manually destroy this because we used placement new to allocate
  rep->~ScopedReport();

}


}  // namespace __tsan

#endif  // SANITIZER_APPLE
