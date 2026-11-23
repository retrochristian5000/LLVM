//===-- tsan_warn_darwin.cpp ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Darwin virtual-memory-layout diagnostics. Keep memory layout checks and
// fatal exit control in the platform implementation.

#include "tsan_warn_darwin.h"

#if SANITIZER_APPLE && !SANITIZER_GO

#include "sanitizer_common/sanitizer_common.h"

namespace __tsan {

void ReportDarwinVirtualMemoryLimit(uptr limit, uptr expected) {
  Report(
      "ThreadSanitizer: Unsupported virtual memory layout:\n\tVM address "
      "limit = %p\n\tExpected %p.\n",
      reinterpret_cast<void *>(limit), reinterpret_cast<void *>(expected));
}

void ReportDarwinOccupiedShadowAddress(uptr address) {
  Report(
      "ThreadSanitizer: Unsupported virtual memory layout: Address %p is "
      "already mapped.\n",
      reinterpret_cast<void *>(address));
}

void ReportDarwinIncompatibleMemoryLayout() {
  Printf("FATAL: ThreadSanitizer: found incompatible memory layout.\n");
}

}  // namespace __tsan

#endif  // SANITIZER_APPLE && !SANITIZER_GO
