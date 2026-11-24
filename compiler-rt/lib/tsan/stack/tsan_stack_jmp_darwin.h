//===-- tsan_stack_jmp_darwin.h -------------------------------*- C++ -*-===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef TSAN_STACK_JMP_DARWIN_H
#define TSAN_STACK_JMP_DARWIN_H
#include "sanitizer_common/sanitizer_platform.h"
#if SANITIZER_APPLE && !SANITIZER_GO
#include "../rtl/tsan_defs.h"
namespace __tsan {
struct JmpBuf;
struct ThreadState;
enum class DarwinJmpBufLookup {
  kFound,
  kNotRecorded,
  kStackPointerMismatch,
  kEnvironmentMismatch,
  kExpiredShadowFrame,
};
// Compare native SPs only within the same signal nesting level: an alternate
// signal stack may be at a higher address than the interrupted stack.
// Distinct envs at the same SP remain live; a reused env replaces its record.
void RetireDarwinJmpBufFrames(ThreadState *thr, uptr stack_pointer,
                              uptr overwritten_env,
                              uptr *shadow_stack_pos, uptr signal_depth);
DarwinJmpBufLookup FindDarwinJmpBufFrame(ThreadState *thr, uptr env,
                                        uptr stack_pointer, JmpBuf **frame);
const char *DarwinJmpBufLookupDescription(DarwinJmpBufLookup status);
}  // namespace __tsan
#endif  // SANITIZER_APPLE && !SANITIZER_GO
#endif  // TSAN_STACK_JMP_DARWIN_H
