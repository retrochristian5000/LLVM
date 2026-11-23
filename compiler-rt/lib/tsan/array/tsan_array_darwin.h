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

#include "../rtl/tsan_defs.h"

namespace __tsan {

// Verify the full page-rounded Mach VM range is in instrumentable app memory.
// Leave zero-sized requests to the kernel.
bool DarwinArrayRangeIsSupported(uptr addr, uptr size);

}  // namespace __tsan

#endif  // SANITIZER_APPLE

#endif  // TSAN_ARRAY_DARWIN_H
