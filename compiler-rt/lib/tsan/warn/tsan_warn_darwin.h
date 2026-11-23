//===-- tsan_warn_darwin.h ------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Darwin-only TSan virtual-memory-layout diagnostic messages. The platform
// caller remains responsible for checking the layout and terminating on error.

#ifndef TSAN_WARN_DARWIN_H
#define TSAN_WARN_DARWIN_H

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE && !SANITIZER_GO

#include "../rtl/tsan_defs.h"
#include "../stack/tsan_stack_jmp_darwin.h"

namespace __tsan {

void ReportDarwinVirtualMemoryLimit(uptr limit, uptr expected);
void ReportDarwinOccupiedShadowAddress(uptr address);
void ReportDarwinIncompatibleMemoryLayout();

void ReportDarwinRejectedFixedMachVmRange(uptr address, uptr size, int flags);
void ReportDarwinUnsupportedMachVmResult(uptr address, uptr size, int flags);
void ReportDarwinInvalidLongJmpStackPointer(uptr stack_pointer);
void ReportDarwinUnmatchedLongJmp(uptr stack_pointer, uptr saved_frames,
                                  uptr environment,
                                  DarwinJmpBufLookup lookup);

}  // namespace __tsan

#endif  // SANITIZER_APPLE && !SANITIZER_GO
#endif  // TSAN_WARN_DARWIN_H
