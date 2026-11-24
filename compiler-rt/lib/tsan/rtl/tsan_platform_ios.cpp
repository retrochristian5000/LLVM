//===-- tsan_platform_ios.cpp ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
// Physical iOS-family device layout policy, including iPadOS. Simulators
// use the desktop-hosted mapping and do not enter this path.
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_IOS && !SANITIZER_IOSSIM && !SANITIZER_GO

#include "sanitizer_common/sanitizer_common.h"
#include "tsan_platform.h"
#include "../warn/tsan_warn_darwin.h"

namespace __tsan {

void ValidateDarwinPlatformLayout(uptr supported_limit) {
  if (supported_limit < HiAppMemEnd()) {
    ReportDarwinVirtualMemoryLimit(supported_limit, HiAppMemEnd());
    Die();
  }
  // MemoryRangeIsAvailable takes an inclusive end. Keep the physical-device
  // boundary probe out of desktop and simulator initialization.
  const uptr last = HiAppMemEnd() - 1;
  if (!MemoryRangeIsAvailable(last, last)) {
    ReportDarwinOccupiedShadowAddress(last);
    Die();
  }
}

void ProtectDarwinDeviceLayout() {
  // Device low application memory and heap are adjacent; a desktop guard
  // between them would overwrite the device heap reservation.
  if (!ReserveDarwinVmRange(HeapMemEnd(), ShadowBeg(), false,
                            "device heap-shadow guard") ||
      !ReserveDarwinVmRange(ShadowEnd(), MetaShadowBeg(), false,
                            "device shadow-meta guard") ||
      !ReserveDarwinVmRange(MetaShadowEnd(), HiAppMemBeg(), false,
                            "device upper guard"))
    Die();
}

}  // namespace __tsan

#endif
