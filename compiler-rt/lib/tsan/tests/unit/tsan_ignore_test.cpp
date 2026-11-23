//===-- tsan_ignore_test.cpp ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tsan_ignore.h"
#include "tsan_rtl.h"
#include "gtest/gtest.h"

namespace __tsan {

TEST(IgnoreScope, NestedMemoryIgnore) {
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

TEST(IgnoreScope, NestedSynchronizationIgnore) {
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

TEST(IgnoreScope, MemoryAndSynchronizationScopesAreIndependent) {
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

#if defined(__aarch64__) && !SANITIZER_GO
TEST(IgnoreScope, AArch64CheckedDepthTransitions) {
  int depth = 0;
  EXPECT_EQ(0, __tsan_ignore_aarch64_depth_step(&depth, -1));
  EXPECT_EQ(0, depth);
  EXPECT_EQ(1, __tsan_ignore_aarch64_depth_step(&depth, 1));
  EXPECT_EQ(1, depth);
  EXPECT_EQ(1, __tsan_ignore_aarch64_depth_step(&depth, -1));
  EXPECT_EQ(0, depth);

  EXPECT_EQ(0, __tsan_ignore_aarch64_depth_step(&depth, 0));
  EXPECT_EQ(0, __tsan_ignore_aarch64_depth_step(&depth, 2));
  EXPECT_EQ(0, depth);
  EXPECT_EQ(0, __tsan_ignore_aarch64_depth_step(nullptr, 1));

  depth = 0x7fffffff;
  EXPECT_EQ(0, __tsan_ignore_aarch64_depth_step(&depth, 1));
  EXPECT_EQ(0x7fffffff, depth);

  depth = -1;
  EXPECT_EQ(0, __tsan_ignore_aarch64_depth_step(&depth, 1));
  EXPECT_EQ(0, __tsan_ignore_aarch64_depth_step(&depth, -1));
  EXPECT_EQ(-1, depth);
}
#endif

}  // namespace __tsan
