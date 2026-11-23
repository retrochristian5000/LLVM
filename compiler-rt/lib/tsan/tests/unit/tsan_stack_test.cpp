//===-- tsan_stack_test.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
//===----------------------------------------------------------------------===//
#include "tsan_sync.h"
#include "tsan_rtl.h"
#include "../../stack/tsan_stack.h"
#include "sanitizer_common/sanitizer_common.h"
#include "gtest/gtest.h"
#if SANITIZER_APPLE && !SANITIZER_GO
#include "../../stack/tsan_stack_jmp_darwin.h"
#endif
#include <string.h>

namespace __tsan {

template <typename StackTraceTy>
static void TestStackTrace(StackTraceTy *trace) {
  ThreadState thr(kMainTid);

  ObtainCurrentStack(&thr, 0, trace);
  EXPECT_EQ(0U, trace->size);

  ObtainCurrentStack(&thr, 42, trace);
  EXPECT_EQ(1U, trace->size);
  EXPECT_EQ(42U, trace->trace[0]);

  *thr.shadow_stack_pos++ = 100;
  *thr.shadow_stack_pos++ = 101;
  ObtainCurrentStack(&thr, 0, trace);
  EXPECT_EQ(2U, trace->size);
  EXPECT_EQ(100U, trace->trace[0]);
  EXPECT_EQ(101U, trace->trace[1]);

  ObtainCurrentStack(&thr, 42, trace);
  EXPECT_EQ(3U, trace->size);
  EXPECT_EQ(100U, trace->trace[0]);
  EXPECT_EQ(101U, trace->trace[1]);
  EXPECT_EQ(42U, trace->trace[2]);
}

template<typename StackTraceTy>
static void TestTrim(StackTraceTy *trace) {
  ThreadState thr(kMainTid);

  for (uptr i = 0; i < 2 * kStackTraceMax; ++i)
    *thr.shadow_stack_pos++ = 100 + i;

  ObtainCurrentStack(&thr, 0, trace);
  EXPECT_EQ(kStackTraceMax, trace->size);
  for (uptr i = 0; i < kStackTraceMax; i++) {
    EXPECT_EQ(100 + kStackTraceMax + i, trace->trace[i]);
  }

  ObtainCurrentStack(&thr, 42, trace);
  EXPECT_EQ(kStackTraceMax, trace->size);
  for (uptr i = 0; i < kStackTraceMax - 1; i++) {
    EXPECT_EQ(101 + kStackTraceMax + i, trace->trace[i]);
  }
  EXPECT_EQ(42U, trace->trace[kStackTraceMax - 1]);
}

#if SANITIZER_APPLE && !SANITIZER_GO
TEST(Stack, DarwinSameDepthJmpBuffersAreIndependent) {
  ThreadState thr(kMainTid);
  JmpBuf *a = thr.jmp_bufs.PushBack();
  a->sp = 0x10000;
  a->env = 0x20000;
  a->shadow_stack_pos = thr.shadow_stack_pos;
  JmpBuf *b = thr.jmp_bufs.PushBack();
  b->sp = 0x10000;
  b->env = 0x30000;
  b->shadow_stack_pos = thr.shadow_stack_pos;

  RetireDarwinJmpBufFrames(&thr, 0x10000, 0);
  ASSERT_EQ(2U, thr.jmp_bufs.Size());
  JmpBuf *found = nullptr;
  EXPECT_EQ(DarwinJmpBufLookup::kFound,
            FindDarwinJmpBufFrame(&thr, 0x20000, 0x10000, &found));
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(0x20000U, found->env);
  EXPECT_EQ(DarwinJmpBufLookup::kEnvironmentMismatch,
            FindDarwinJmpBufFrame(&thr, 0x40000, 0x10000, &found));
  EXPECT_EQ(nullptr, found);
  EXPECT_EQ(DarwinJmpBufLookup::kStackPointerMismatch,
            FindDarwinJmpBufFrame(&thr, 0x30000, 0x11000, &found));
  EXPECT_EQ(DarwinJmpBufLookup::kNotRecorded,
            FindDarwinJmpBufFrame(&thr, 0x40000, 0x11000, &found));

  RetireDarwinJmpBufFrames(&thr, 0x10000, 0x20000);
  ASSERT_EQ(1U, thr.jmp_bufs.Size());
  EXPECT_EQ(0x30000U, thr.jmp_bufs[0].env);
}

TEST(Stack, DarwinJmpFrameLifetimeAndShadowDepth) {
  ThreadState thr(kMainTid);
  JmpBuf *outer = thr.jmp_bufs.PushBack();
  outer->sp = 0x20000;
  outer->env = 0x30000;
  outer->shadow_stack_pos = thr.shadow_stack_pos;
  JmpBuf *inner = thr.jmp_bufs.PushBack();
  inner->sp = 0x1f000;
  inner->env = 0x31000;
  inner->shadow_stack_pos = thr.shadow_stack_pos + 2;

  JmpBuf *found = nullptr;
  EXPECT_EQ(DarwinJmpBufLookup::kExpiredShadowFrame,
            FindDarwinJmpBufFrame(&thr, 0x31000, 0x1f000, &found));
  EXPECT_EQ(nullptr, found);

  RetireDarwinJmpBufFrames(&thr, 0x20000, 0);
  ASSERT_EQ(1U, thr.jmp_bufs.Size());
  EXPECT_EQ(DarwinJmpBufLookup::kFound,
            FindDarwinJmpBufFrame(&thr, 0x30000, 0x20000, &found));
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(0x30000U, found->env);
}
#endif

TEST(Stack, ShadowStackAllocationAndRelease) {
  ThreadState thr(kMainTid);
  ASSERT_NE(nullptr, thr.shadow_stack);
  EXPECT_EQ(thr.shadow_stack, thr.shadow_stack_pos);
  EXPECT_EQ(kShadowStackSize,
            static_cast<uptr>(thr.shadow_stack_end - thr.shadow_stack));

  DestroyShadowStack(&thr);
  EXPECT_EQ(nullptr, thr.shadow_stack);
  EXPECT_EQ(nullptr, thr.shadow_stack_pos);
  EXPECT_EQ(nullptr, thr.shadow_stack_end);
}

#if SANITIZER_APPLE && !SANITIZER_GO
TEST(Stack, DarwinEmptyInitializationIsNoOp) {
  ThreadState thr(kMainTid);

  thr.stk_addr = 0;
  thr.stk_size = 4096;
  ImitateDarwinStackInitialization(&thr);

  thr.stk_addr = 4096;
  thr.stk_size = 0;
  ImitateDarwinStackInitialization(&thr);
}
#endif

#if SANITIZER_OSX && !SANITIZER_GO
TEST(Stack, MacCachedStackBoundsValidation) {
  const uptr begin = 0x10000;
  const uptr size = 0x4000;
  uptr top = 0;
  uptr bottom = 0;

  EXPECT_TRUE(TryMacCachedStackBounds(begin, size, begin + 0x1000,
                                     begin + 0x2000, &top, &bottom));
  EXPECT_EQ(begin, bottom);
  EXPECT_EQ(begin + size, top);

  // A frame or currently executing stack outside this pthread stack must
  // use the generic query instead (signals and fibers may switch stacks).
  EXPECT_FALSE(TryMacCachedStackBounds(begin, size, begin + size,
                                      begin + 0x2000, &top, &bottom));
  EXPECT_FALSE(TryMacCachedStackBounds(begin, size, begin + 0x1000,
                                      begin - 1, &top, &bottom));
  EXPECT_FALSE(TryMacCachedStackBounds(begin, size, 0,
                                      begin + 0x2000, &top, &bottom));
  EXPECT_FALSE(TryMacCachedStackBounds(0, size, begin + 0x1000,
                                      begin + 0x2000, &top, &bottom));
  EXPECT_FALSE(TryMacCachedStackBounds(begin, 0, begin + 0x1000,
                                      begin + 0x2000, &top, &bottom));
  EXPECT_FALSE(TryMacCachedStackBounds(~static_cast<uptr>(0) - 7, 16,
                                      begin + 0x1000, begin + 0x2000,
                                      &top, &bottom));
  // Failed fast-path checks must not modify the fallback's output slots.
  EXPECT_EQ(begin, bottom);
  EXPECT_EQ(begin + size, top);
}

TEST(Stack, MacStackBoundsMatchPthreadFallback) {
  uptr expected_top = 0;
  uptr expected_bottom = 0;
  GetThreadStackTopAndBottom(false, &expected_top, &expected_bottom);

  uptr cached_top = 0;
  uptr cached_bottom = 0;
  // The main thread deliberately uses the OS query, matching its historical
  // initialization behavior. Other non-cached cases use the same fallback.
  GetMacStackBounds(reinterpret_cast<uptr>(&cached_top), &cached_top,
                    &cached_bottom);
  EXPECT_EQ(expected_top, cached_top);
  EXPECT_EQ(expected_bottom, cached_bottom);
}
#endif

TEST(StackTrace, BasicVarSize) {
  VarSizeStackTrace trace;
  TestStackTrace(&trace);
}

TEST(StackTrace, BasicBuffered) {
  BufferedStackTrace trace;
  TestStackTrace(&trace);
}

TEST(StackTrace, TrimVarSize) {
  VarSizeStackTrace trace;
  TestTrim(&trace);
}

TEST(StackTrace, TrimBuffered) {
  BufferedStackTrace trace;
  TestTrim(&trace);
}

}  // namespace __tsan
