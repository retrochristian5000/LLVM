//===-- tsan_warn_runtime.cpp ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Structured, allocation-free fatal diagnostics for TSan runtime failures.

#include "tsan_warn_runtime.h"

#include "sanitizer_common/sanitizer_common.h"

namespace __tsan {

const char *AppMemoryRangeErrorDescription(AppMemoryRangeError error) {
  switch (error) {
    case AppMemoryRangeError::kNone:
      return "none";
    case AppMemoryRangeError::kEmpty:
      return "zero-sized memory range";
    case AppMemoryRangeError::kOverflow:
      return "address plus length overflows the address space";
    case AppMemoryRangeError::kOutsideAppMemory:
      return "starting address is outside instrumentable application memory";
    case AppMemoryRangeError::kCrossesGap:
      return "range crosses a gap between instrumentable memory windows";
  }
  return "unrecognized memory range error";
}

void ReportUnexpectedMmapRange(uptr requested, uptr returned, uptr size) {
  uptr first_unsupported = 0;
  const AppMemoryRangeError error =
      DiagnoseAppMemoryRange(returned, size, &first_unsupported);
  Report(
      "FATAL: ThreadSanitizer [TSAN-MAP-001]: mmap returned an unsupported "
      "application-memory range\n"
      "  operation=mmap requested_address=%p returned_address=%p "
      "size_bytes=%zu\n"
      "  reason=%s first_unsupported_address=%p\n"
      "  action=verify the target virtual-memory layout and address ranges; "
      "do not suppress actual data races\n",
      reinterpret_cast<void *>(requested), reinterpret_cast<void *>(returned),
      size, AppMemoryRangeErrorDescription(error),
      reinterpret_cast<void *>(first_unsupported));
}

void ReportInvalidSetJmpStackPointer(uptr stack_pointer) {
  Report(
      "FATAL: ThreadSanitizer [TSAN-JMP-001]: invalid AArch64 setjmp "
      "stack pointer\n"
      "  operation=setjmp stack_pointer=%p required_alignment_bytes=16\n"
      "  reason=stack pointer is null or fails the AArch64 alignment check\n"
      "  action=inspect setjmp interception and the calling ABI\n",
      reinterpret_cast<void *>(stack_pointer));
}

}  // namespace __tsan
