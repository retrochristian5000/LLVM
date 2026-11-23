//===-- tsan_warn_runtime.h -----------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Fatal TSan runtime diagnostics. Keep detection and error policy in callers;
// these functions only describe already-detected failures.

#ifndef TSAN_WARN_RUNTIME_H
#define TSAN_WARN_RUNTIME_H

#include "../rtl/tsan_platform.h"

namespace __tsan {

const char *AppMemoryRangeErrorDescription(AppMemoryRangeError error);

// Report a successful mmap whose returned address cannot be instrumented.
// The caller remains responsible for terminating after this diagnostic.
void ReportUnexpectedMmapRange(uptr requested, uptr returned, uptr size);

// AArch64 setjmp captured a non-null/alignment-invalid stack pointer.
void ReportInvalidSetJmpStackPointer(uptr stack_pointer);

}  // namespace __tsan

#endif  // TSAN_WARN_RUNTIME_H
