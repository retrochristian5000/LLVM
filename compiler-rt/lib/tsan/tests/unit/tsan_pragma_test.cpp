//===-- tsan_pragma_test.cpp ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tsan_pragma.h"
#include "tsan_rtl.h"
#include "gtest/gtest.h"

namespace __tsan {

TEST(PragmaScope, NestedMemoryIgnore) {
  ThreadState thr(kMainTid);
  thr.ignore_reads_and_writes = 0;
  EXPECT_FALSE(thr.fast_state.GetIgnoreBit());

  ThreadIgnoreBegin(&thr, 0);
  EXPECT_EQ(1, thr.ignore_reads_and_writes);
  EXPECT_TRUE(thr.fast_state.GetIgnoreBit());

  ThreadIgnoreBegin(&thr, 0);
  EXPECT_EQ(2, thr.ignore_reads_and_writes);
  ThreadIgnoreEnd(&thr);
  EXPECT_EQ(1, thr.ignore_reads_and_writes);
  EXPECT_TRUE(thr.fast_state.GetIgnoreBit());

  ThreadIgnoreEnd(&thr);
  EXPECT_EQ(0, thr.ignore_reads_and_writes);
  EXPECT_FALSE(thr.fast_state.GetIgnoreBit());
  DestroyShadowStack(&thr);
}

TEST(PragmaScope, NestedSynchronizationIgnore) {
  ThreadState thr(kMainTid);
  thr.ignore_sync = 0;

  ThreadIgnoreSyncBegin(&thr, 0);
  ThreadIgnoreSyncBegin(&thr, 0);
  EXPECT_EQ(2, thr.ignore_sync);

  ThreadIgnoreSyncEnd(&thr);
  EXPECT_EQ(1, thr.ignore_sync);
  ThreadIgnoreSyncEnd(&thr);
  EXPECT_EQ(0, thr.ignore_sync);
  DestroyShadowStack(&thr);
}

TEST(PragmaScope, MemoryAndSynchronizationScopesAreIndependent) {
  ThreadState thr(kMainTid);
  thr.ignore_reads_and_writes = 0;
  thr.ignore_sync = 0;

  ThreadIgnoreBegin(&thr, 0);
  ThreadIgnoreSyncBegin(&thr, 0);
  ThreadIgnoreEnd(&thr);
  EXPECT_EQ(0, thr.ignore_reads_and_writes);
  EXPECT_FALSE(thr.fast_state.GetIgnoreBit());
  EXPECT_EQ(1, thr.ignore_sync);

  ThreadIgnoreSyncEnd(&thr);
  EXPECT_EQ(0, thr.ignore_sync);
  DestroyShadowStack(&thr);
}

}  // namespace __tsan
