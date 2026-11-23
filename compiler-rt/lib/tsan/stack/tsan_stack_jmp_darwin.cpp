//===-- tsan_stack_jmp_darwin.cpp ----------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "tsan_stack_jmp_darwin.h"
#if SANITIZER_APPLE && !SANITIZER_GO
#include "../rtl/tsan_rtl.h"
namespace __tsan {
void RetireDarwinJmpBufFrames(ThreadState *thr, uptr stack_pointer,
                              uptr overwritten_env) {
  for (uptr i = 0; i < thr->jmp_bufs.Size();) {
    const JmpBuf &frame = thr->jmp_bufs[i];
    // Deeper returned frames expire; equal-SP environments do not.
    const bool expired = frame.sp < stack_pointer ||
                         (overwritten_env && frame.env == overwritten_env);
    if (!expired) {
      ++i;
      continue;
    }
    const uptr last = thr->jmp_bufs.Size() - 1;
    if (i != last)
      thr->jmp_bufs[i] = thr->jmp_bufs[last];
    thr->jmp_bufs.PopBack();
  }
}
DarwinJmpBufLookup FindDarwinJmpBufFrame(ThreadState *thr, uptr env,
                                        uptr stack_pointer, JmpBuf **frame) {
  *frame = nullptr;
  bool found_env = false;
  bool found_stack_pointer = false;
  for (uptr i = thr->jmp_bufs.Size(); i != 0; --i) {
    JmpBuf *candidate = &thr->jmp_bufs[i - 1];
    if (candidate->env != env) {
      if (candidate->sp == stack_pointer)
        found_stack_pointer = true;
      continue;
    }
    found_env = true;
    if (candidate->sp != stack_pointer)
      continue;
    const uptr saved_pos = reinterpret_cast<uptr>(candidate->shadow_stack_pos);
    const uptr begin = reinterpret_cast<uptr>(thr->shadow_stack);
    const uptr current_pos = reinterpret_cast<uptr>(thr->shadow_stack_pos);
    if (!begin || saved_pos < begin || saved_pos > current_pos)
      return DarwinJmpBufLookup::kExpiredShadowFrame;
    *frame = candidate;
    return DarwinJmpBufLookup::kFound;
  }
  if (found_env)
    return DarwinJmpBufLookup::kStackPointerMismatch;
  if (found_stack_pointer)
    return DarwinJmpBufLookup::kEnvironmentMismatch;
  return DarwinJmpBufLookup::kNotRecorded;
}
const char *DarwinJmpBufLookupDescription(DarwinJmpBufLookup status) {
  switch (status) {
    case DarwinJmpBufLookup::kFound:
      return "matching live setjmp frame";
    case DarwinJmpBufLookup::kNotRecorded:
      return "no setjmp frame is recorded for this environment";
    case DarwinJmpBufLookup::kStackPointerMismatch:
      return "saved environment has a different decoded stack pointer";
    case DarwinJmpBufLookup::kEnvironmentMismatch:
      return "decoded stack pointer belongs to a different jump buffer";
    case DarwinJmpBufLookup::kExpiredShadowFrame:
      return "saved shadow frame is no longer on the active shadow stack";
  }
  return "unrecognized jump-buffer lookup state";
}
}  // namespace __tsan
#endif  // SANITIZER_APPLE && !SANITIZER_GO
