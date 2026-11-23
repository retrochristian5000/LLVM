//===-- tsan_buff_test.cpp ------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tsan_buff.h"
#include "tsan_stack_trace.h"
#include "gtest/gtest.h"

namespace __tsan {

TEST(Buff, ReverseEmptyAndSingleEntry) {
  ReversePcBuffer(nullptr, 0);
  uptr one[] = {42};
  ReversePcBuffer(one, 1);
  EXPECT_EQ(42U, one[0]);
}

TEST(Buff, ReverseOddAndEvenEntries) {
  uptr odd[] = {11, 22, 33};
  ReversePcBuffer(odd, 3);
  EXPECT_EQ(33U, odd[0]);
  EXPECT_EQ(22U, odd[1]);
  EXPECT_EQ(11U, odd[2]);

  uptr even[] = {10, 20, 30, 40};
  ReversePcBuffer(even, 4);
  EXPECT_EQ(40U, even[0]);
  EXPECT_EQ(30U, even[1]);
  EXPECT_EQ(20U, even[2]);
  EXPECT_EQ(10U, even[3]);
}

TEST(Buff, VarSizeTraceInitializeReverseAndClear) {
  VarSizeStackTrace trace;
  trace.Init(nullptr, 0);
  EXPECT_EQ(0U, trace.size);
  EXPECT_EQ(nullptr, trace.trace_buffer);

  const uptr pcs[] = {100, 200, 300};
  trace.Init(pcs, 3, 400);
  ASSERT_EQ(4U, trace.size);
  ASSERT_NE(nullptr, trace.trace_buffer);
  EXPECT_EQ(100U, trace.trace[0]);
  EXPECT_EQ(400U, trace.trace[3]);

  trace.ReverseOrder();
  EXPECT_EQ(400U, trace.trace[0]);
  EXPECT_EQ(300U, trace.trace[1]);
  EXPECT_EQ(200U, trace.trace[2]);
  EXPECT_EQ(100U, trace.trace[3]);

  trace.Init(pcs, 1);
  ASSERT_EQ(1U, trace.size);
  EXPECT_EQ(100U, trace.trace[0]);

  trace.Init(nullptr, 0);
  EXPECT_EQ(0U, trace.size);
  EXPECT_EQ(nullptr, trace.trace_buffer);
  EXPECT_EQ(nullptr, trace.trace);
}

}  // namespace __tsan
