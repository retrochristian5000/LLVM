//===-- tsan_warn.cpp -----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// TSan warning policy and emission: report hooks, suppression decisions,
// warning counts, and the final warning summary. Race reconstruction and
// stack symbolization remain in tsan_rtl_report.cpp, while PrintReport()
// owns the warning's detailed formatting.

#include "sanitizer_common/sanitizer_atomic.h"
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_internal_defs.h"
#include "../flags/tsan_flags.h"
#include "tsan_report.h"
#include "tsan_rtl.h"
#include "tsan_suppressions.h"

namespace __tsan {

using namespace __sanitizer;

// Can be overriden by an application/test to intercept reports.
#ifdef TSAN_EXTERNAL_HOOKS
bool OnReport(const ReportDesc *rep, bool suppressed);
#else
SANITIZER_WEAK_CXX_DEFAULT_IMPL
bool OnReport(const ReportDesc *rep, bool suppressed) {
  (void)rep;
  return suppressed;
}
#endif

SANITIZER_WEAK_DEFAULT_IMPL
void __tsan_on_report(const ReportDesc *rep) {
  (void)rep;
}

bool ShouldReport(ThreadState *thr, ReportType typ) {
  // We set thr->suppress_reports in the fork context.
  // Taking any locking in the fork context can lead to deadlocks.
  // If any locks are already taken, it's too late to do this check.
  CheckedMutex::CheckNoLocks();
  // For the same reason check we didn't lock thread_registry yet.
  if (SANITIZER_DEBUG)
    ThreadRegistryLock l(&ctx->thread_registry);
  if (!flags()->report_bugs || thr->suppress_reports)
    return false;
  switch (typ) {
    case ReportTypeSignalUnsafe:
      return flags()->report_signal_unsafe;
    case ReportTypeThreadLeak:
#if !SANITIZER_GO
      // It's impossible to join phantom threads
      // in the child after fork.
      if (ctx->after_multithreaded_fork)
        return false;
#endif
      return flags()->report_thread_leaks;
    case ReportTypeMutexDestroyLocked:
      return flags()->report_destroy_locked;
    default:
      return true;
  }
}

bool OutputReport(ThreadState *thr, ScopedReport &srep) {
  // These should have been checked in ShouldReport.
  // It's too late to check them here, we have already taken locks.
  CHECK(flags()->report_bugs);
  CHECK(!thr->suppress_reports);
  srep.SymbolizeStackElems();
  atomic_store_relaxed(&ctx->last_symbolize_time_ns, NanoTime());
  const ReportDesc *rep = srep.GetReport();
  CHECK_EQ(thr->current_report, nullptr);
  thr->current_report = rep;
  Suppression *supp = 0;
  uptr pc_or_addr = 0;
  for (uptr i = 0; pc_or_addr == 0 && i < rep->mops.Size(); i++)
    pc_or_addr = IsSuppressed(rep->typ, rep->mops[i]->stack, &supp);
  for (uptr i = 0; pc_or_addr == 0 && i < rep->stacks.Size(); i++)
    pc_or_addr = IsSuppressed(rep->typ, rep->stacks[i], &supp);
  for (uptr i = 0; pc_or_addr == 0 && i < rep->threads.Size(); i++)
    pc_or_addr = IsSuppressed(rep->typ, rep->threads[i]->stack, &supp);
  for (uptr i = 0; pc_or_addr == 0 && i < rep->locs.Size(); i++)
    pc_or_addr = IsSuppressed(rep->typ, rep->locs[i], &supp);
  if (pc_or_addr != 0) {
    Lock lock(&ctx->fired_suppressions_mtx);
    FiredSuppression s = {srep.GetReport()->typ, pc_or_addr, supp};
    ctx->fired_suppressions.push_back(s);
  }
  {
    bool suppressed = OnReport(rep, pc_or_addr != 0);
    if (suppressed) {
      thr->current_report = nullptr;
      return false;
    }
  }
  PrintReport(rep);
  __tsan_on_report(rep);
  ctx->nreported++;
  if (flags()->halt_on_error)
    Die();
  thr->current_report = nullptr;
  return true;
}

bool PrintWarningSummary() {
  const bool failed = ctx->nreported != 0;
  if (failed) {
#if !SANITIZER_GO
    Printf("ThreadSanitizer: reported %d warnings\n", ctx->nreported);
#else
    Printf("Found %d data race(s)\n", ctx->nreported);
#endif
  }

  if (common_flags()->print_suppressions)
    PrintMatchedSuppressions();

  return failed;
}

}  // namespace __tsan
