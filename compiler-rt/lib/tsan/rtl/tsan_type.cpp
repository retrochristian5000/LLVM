//===-- tsan_type.cpp -----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Race report classification and report-type descriptions.
// Access flag definitions and the memory-access hot path remain unchanged.
//
//===----------------------------------------------------------------------===//

#include "tsan_type.h"
#include "tsan_rtl.h"

namespace __tsan {

ReportType RaceReportType(AccessType current, AccessType previous) {
  if ((current & kAccessVptr) && (previous & kAccessFree))
    return ReportTypeVptrUseAfterFree;
  if (current & kAccessVptr)
    return ReportTypeVptrRace;
  if (previous & kAccessFree)
    return ReportTypeUseAfterFree;
  return ReportTypeRace;
}

#if !SANITIZER_GO

const char *ReportTypeString(ReportType typ, uptr tag) {
  switch (typ) {
    case ReportTypeRace:
      return "data race";
    case ReportTypeVptrRace:
      return "data race on vptr (ctor/dtor vs virtual call)";
    case ReportTypeUseAfterFree:
      return "heap-use-after-free";
    case ReportTypeVptrUseAfterFree:
      return "heap-use-after-free (virtual call vs free)";
    case ReportTypeExternalRace: {
      const char *str = GetReportHeaderFromTag(tag);
      return str ? str : "race on external object";
    }
    case ReportTypeThreadLeak:
      return "thread leak";
    case ReportTypeMutexDestroyLocked:
      return "destroy of a locked mutex";
    case ReportTypeMutexDoubleLock:
      return "double lock of a mutex";
    case ReportTypeMutexInvalidAccess:
      return "use of an invalid mutex (e.g. uninitialized or destroyed)";
    case ReportTypeMutexBadUnlock:
      return "unlock of an unlocked mutex (or by a wrong thread)";
    case ReportTypeMutexBadReadLock:
      return "read lock of a write locked mutex";
    case ReportTypeMutexBadReadUnlock:
      return "read unlock of a write locked mutex";
    case ReportTypeSignalUnsafe:
      return "signal-unsafe call inside of a signal";
    case ReportTypeErrnoInSignal:
      return "signal handler spoils errno";
    case ReportTypeDeadlock:
      return "lock-order-inversion (potential deadlock)";
    case ReportTypeMutexHeldWrongContext:
      return "mutex held in the wrong context";
      // No default case so compiler warns us if we miss one
  }
  UNREACHABLE("missing case");
}

#endif  // !SANITIZER_GO

}  // namespace __tsan
