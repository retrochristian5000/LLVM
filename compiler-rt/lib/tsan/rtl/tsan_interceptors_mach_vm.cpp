//===-- tsan_interceptors_mach_vm.cpp -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
// Interceptors for mach_vm_* user space memory routines on Darwin.
//===----------------------------------------------------------------------===//

#include "interception/interception.h"
#include "tsan_array_darwin.h"
#include "tsan_interceptors.h"
#include "tsan_platform.h"
#include "../warn/tsan_warn_darwin.h"

#include <mach/mach.h>

namespace __tsan {

TSAN_INTERCEPTOR(kern_return_t, mach_vm_allocate, vm_map_t target,
                 mach_vm_address_t *address, mach_vm_size_t size, int flags) {
  SCOPED_TSAN_INTERCEPTOR(mach_vm_allocate, target, address, size, flags);
  if (target != mach_task_self())
    return REAL(mach_vm_allocate)(target, address, size, flags);
  // Fixed mappings may not overlap TSan's reserved shadow regions.
  if (address && !(flags & VM_FLAGS_ANYWHERE) &&
      !DarwinArrayRangeIsSupported(*address, size)) {
    ReportDarwinRejectedFixedMachVmRange(*address, size, flags);
    return KERN_NO_SPACE;
  }
  kern_return_t kr = REAL(mach_vm_allocate)(target, address, size, flags);
  if (kr == KERN_SUCCESS && address && size != 0) {
    // Anywhere allocations must also stay inside instrumented app memory.
    if (!DarwinArrayRangeIsSupported(*address, size)) {
      ReportDarwinUnsupportedMachVmResult(*address, size, flags);
      Die();
    }
    MemoryRangeImitateWriteOrResetRange(thr, pc, *address, size);
  }
  return kr;
}

TSAN_INTERCEPTOR(kern_return_t, mach_vm_deallocate, vm_map_t target,
                 mach_vm_address_t address, mach_vm_size_t size) {
  SCOPED_TSAN_INTERCEPTOR(mach_vm_deallocate, target, address, size);
  if (target != mach_task_self())
    return REAL(mach_vm_deallocate)(target, address, size);
  // Unlike POSIX munmap, Mach accepts an unaligned start and rounds it
  // down. Clear the same complete page interval that the kernel removes.
  if (size) {
    const DarwinArrayRangeDiagnosis range =
        DiagnoseDarwinArrayRange(address, size);
    if (range.error == AppMemoryRangeError::kNone)
      UnmapShadow(thr, range.page_begin, range.page_end - range.page_begin);
  }
  return REAL(mach_vm_deallocate)(target, address, size);
}

}  // namespace __tsan
