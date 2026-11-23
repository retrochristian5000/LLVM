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
#include "../array/tsan_array_darwin.h"
#include "tsan_warn_runtime.h"

namespace __tsan {

void ReportDarwinVirtualMemoryLimit(uptr limit, uptr expected) {
  Report(
      "FATAL: ThreadSanitizer [TSAN-LAYOUT-001]: virtual-memory limit is "
      "below the required application-memory boundary\n"
      "  platform=Darwin virtual_address_limit=%p required_boundary=%p\n"
      "  action=verify the OS virtual-address configuration and TSan "
      "architecture mapping\n",
      reinterpret_cast<void *>(limit), reinterpret_cast<void *>(expected));
}

void ReportDarwinOccupiedShadowAddress(uptr address) {
  Report(
      "FATAL: ThreadSanitizer [TSAN-LAYOUT-002]: reserved TSan memory "
      "address is already mapped\n"
      "  platform=Darwin conflicting_address=%p\n"
      "  action=inspect the existing VM mappings and TSan shadow layout\n",
      reinterpret_cast<void *>(address));
}

void ReportDarwinIncompatibleMemoryLayout() {
  Report(
      "FATAL: ThreadSanitizer [TSAN-LAYOUT-003]: incompatible "
      "virtual-memory layout\n"
      "  platform=Darwin operation=CheckAndProtect\n"
      "  action=inspect ASLR, mapped images, and the TSan reserved-memory "
      "windows\n");
}

void ReportDarwinRejectedFixedMachVmRange(uptr address, uptr size, int flags) {
  const DarwinArrayRangeDiagnosis diagnosis =
      DiagnoseDarwinArrayRange(address, size);
  VPrintf(
      1, "ThreadSanitizer [TSAN-VM-001]: rejected fixed mach_vm_allocate "
         "outside supported application memory\n"
         "  operation=mach_vm_allocate requested_address=%p size_bytes=%zu "
         "flags=0x%x\n"
         "  reason=%s page_begin=%p page_end=%p "
         "first_unsupported_address=%p\n",
      reinterpret_cast<void *>(address), size, flags,
      AppMemoryRangeErrorDescription(diagnosis.error),
      reinterpret_cast<void *>(diagnosis.page_begin),
      reinterpret_cast<void *>(diagnosis.page_end),
      reinterpret_cast<void *>(diagnosis.first_unsupported));
}

void ReportDarwinUnsupportedMachVmResult(uptr address, uptr size, int flags) {
  const DarwinArrayRangeDiagnosis diagnosis =
      DiagnoseDarwinArrayRange(address, size);
  Report(
      "FATAL: ThreadSanitizer [TSAN-VM-002]: mach_vm_allocate returned "
      "an unsupported application-memory range\n"
      "  operation=mach_vm_allocate returned_address=%p size_bytes=%zu "
      "flags=0x%x\n"
      "  reason=%s page_begin=%p page_end=%p "
      "first_unsupported_address=%p\n"
      "  action=verify the Darwin VM layout and page-rounded allocation; "
      "do not bypass TSan shadow checks\n",
      reinterpret_cast<void *>(address), size, flags,
      AppMemoryRangeErrorDescription(diagnosis.error),
      reinterpret_cast<void *>(diagnosis.page_begin),
      reinterpret_cast<void *>(diagnosis.page_end),
      reinterpret_cast<void *>(diagnosis.first_unsupported));
}

void ReportDarwinInvalidLongJmpStackPointer(uptr stack_pointer) {
  Report(
      "FATAL: ThreadSanitizer [TSAN-JMP-002]: invalid decoded Darwin "
      "AArch64 longjmp stack pointer\n"
      "  operation=longjmp decoded_stack_pointer=%p "
      "required_alignment_bytes=16\n"
      "  reason=decoded pointer is null or fails the AArch64 alignment check\n"
      "  action=inspect the libc jump-buffer layout and pointer authentication; "
      "an authentication failure may trap before this check\n",
      reinterpret_cast<void *>(stack_pointer));
}

void ReportDarwinUnmatchedLongJmp(uptr stack_pointer, uptr saved_frames,
                                  uptr environment,
                                  DarwinJmpBufLookup lookup) {
  Report(
      "FATAL: ThreadSanitizer [TSAN-JMP-003]: longjmp has no matching "
      "live setjmp frame\n"
      "  operation=longjmp environment=%p decoded_stack_pointer=%p "
      "saved_frame_count=%zu\n"
      "  reason=%s\n"
      "  action=inspect jump-buffer identity, frame lifetime, and Darwin "
      "jump-buffer ABI; verify arm64e pointer authentication if applicable\n",
      reinterpret_cast<void *>(environment),
      reinterpret_cast<void *>(stack_pointer), saved_frames,
      DarwinJmpBufLookupDescription(lookup));
}

}  // namespace __tsan

#endif  // SANITIZER_APPLE && !SANITIZER_GO
