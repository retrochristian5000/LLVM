//===-- tsan_type_test.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tsan_type.h"
#include "gtest/gtest.h"

namespace __tsan {

TEST(Type, RaceClassification) {
  EXPECT_EQ(ReportTypeRace, RaceReportType(kAccessWrite, kAccessRead));
  EXPECT_EQ(ReportTypeRace,
            RaceReportType(kAccessRead | kAccessAtomic, kAccessWrite));
  EXPECT_EQ(ReportTypeVptrRace,
            RaceReportType(kAccessVptr | kAccessRead, kAccessWrite));
  EXPECT_EQ(ReportTypeUseAfterFree,
            RaceReportType(kAccessWrite, kAccessFree));
  EXPECT_EQ(ReportTypeVptrUseAfterFree,
            RaceReportType(kAccessVptr | kAccessRead, kAccessFree));
  // The previous access's vptr flag does not change classification.
  EXPECT_EQ(ReportTypeRace,
            RaceReportType(kAccessWrite, kAccessVptr));
  // A free in the current access is not the previous-access free marker.
  EXPECT_EQ(ReportTypeRace,
            RaceReportType(kAccessFree, kAccessRead));
}

#if !SANITIZER_GO

TEST(Type, ReportNames) {
  struct Case {
    ReportType type;
    const char *name;
  };
  const Case cases[] = {
      {ReportTypeRace, "data race"},
      {ReportTypeVptrRace, "data race on vptr (ctor/dtor vs virtual call)"},
      {ReportTypeUseAfterFree, "heap-use-after-free"},
      {ReportTypeVptrUseAfterFree,
       "heap-use-after-free (virtual call vs free)"},
      {ReportTypeExternalRace, "race on external object"},
      {ReportTypeThreadLeak, "thread leak"},
      {ReportTypeMutexDestroyLocked, "destroy of a locked mutex"},
      {ReportTypeMutexDoubleLock, "double lock of a mutex"},
      {ReportTypeMutexInvalidAccess,
       "use of an invalid mutex (e.g. uninitialized or destroyed)"},
      {ReportTypeMutexBadUnlock,
       "unlock of an unlocked mutex (or by a wrong thread)"},
      {ReportTypeMutexBadReadLock, "read lock of a write locked mutex"},
      {ReportTypeMutexBadReadUnlock, "read unlock of a write locked mutex"},
      {ReportTypeSignalUnsafe, "signal-unsafe call inside of a signal"},
      {ReportTypeErrnoInSignal, "signal handler spoils errno"},
      {ReportTypeDeadlock, "lock-order-inversion (potential deadlock)"},
      {ReportTypeMutexHeldWrongContext, "mutex held in the wrong context"},
  };

  for (const Case &c : cases)
    EXPECT_STREQ(c.name, ReportTypeString(c.type, kExternalTagNone));
}

#endif  // !SANITIZER_GO

}  // namespace __tsan
