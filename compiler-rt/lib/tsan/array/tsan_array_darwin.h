//===-- tsan_array_darwin.h ------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_ARRAY_DARWIN_H
#define TSAN_ARRAY_DARWIN_H

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE

#include "../rtl/tsan_platform.h"

namespace __tsan {

struct DarwinArrayRangeDiagnosis {
  AppMemoryRangeError error;
  uptr page_begin;
  uptr page_end;
  uptr first_unsupported;
};

// Diagnose the entire page-rounded Mach VM interval, including alignment and
// address overflow. Zero-sized requests are left to the Darwin kernel.
DarwinArrayRangeDiagnosis DiagnoseDarwinArrayRange(uptr addr, uptr size);

bool DarwinArrayRangeIsSupported(uptr addr, uptr size);

// XNU fixed allocations realign the start before adding size, unlike
// deallocation. The returned address of an anywhere allocation is aligned.
DarwinArrayRangeDiagnosis DiagnoseDarwinAllocationRange(uptr addr, uptr size);

}  // namespace __tsan

#endif  // SANITIZER_APPLE

#endif  // TSAN_ARRAY_DARWIN_H
