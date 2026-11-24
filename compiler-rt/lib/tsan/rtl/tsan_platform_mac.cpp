//===-- tsan_platform_mac.cpp ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
// Desktop-hosted Darwin layout validation (macOS and simulators).
// Physical iOS/iPadOS device layout validation lives in tsan_platform_ios.cpp.
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE && !SANITIZER_GO && (!SANITIZER_IOS || SANITIZER_IOSSIM)

#include "sanitizer_common/sanitizer_common.h"
#include "tsan_platform.h"
#include "../warn/tsan_warn_darwin.h"

namespace __tsan {

void ValidateDarwinPlatformLayout(uptr supported_limit) {
  // The desktop layout only reserves through the start of the high
  // application window. A kernel-reserved tail need not be available.
  if (supported_limit < HiAppMemBeg()) {
    ReportDarwinVirtualMemoryLimit(supported_limit, HiAppMemBeg());
    Die();
  }
}

}  // namespace __tsan

#endif
