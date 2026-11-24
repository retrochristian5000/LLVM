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
  // XNU fixed allocations round the start down before adding size.
  // Keep the existing conservative check for explicit superpage requests;
  // their extent can depend on additional kernel alignment rules.
  const bool ordinary_pages = !(flags & VM_FLAGS_SUPERPAGE_MASK);
  if (address && !(flags & VM_FLAGS_ANYWHERE) &&
      (ordinary_pages
           ? DiagnoseDarwinAllocationRange(*address, size).error !=
                 AppMemoryRangeError::kNone
           : !DarwinArrayRangeIsSupported(*address, size))) {
    ReportDarwinRejectedFixedMachVmRange(*address, size, flags);
    return KERN_NO_SPACE;
  }
  kern_return_t kr = REAL(mach_vm_allocate)(target, address, size, flags);
  if (kr == KERN_SUCCESS && address && size != 0) {
    // Anywhere allocations must also stay inside instrumented app memory.
    const DarwinArrayRangeDiagnosis range =
        ordinary_pages ? DiagnoseDarwinAllocationRange(*address, size)
                       : DiagnoseDarwinArrayRange(*address, size);
    if (range.error != AppMemoryRangeError::kNone) {
      ReportDarwinUnsupportedMachVmResult(*address, size, flags);
      Die();
    }
    // Ordinary allocations initialize whole pages, not just the requested
    // bytes. In particular a fixed overwrite must not retain stale shadow
    // records in the tail of its last page.
    if (ordinary_pages)
      MemoryRangeImitateWriteOrResetRange(thr, pc, range.page_begin,
                                         range.page_end - range.page_begin);
    else
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


#if SANITIZER_OSX
namespace {

// Account for the logical byte transfer, not for page-rounded allocation.
// Recheck shadow translation at cell boundaries: adjacent app windows do
// not necessarily have contiguous shadow addresses, and a read starting
// in a read-only cell must not hide later writable cells.
void TrackMachCopyRange(ThreadState *thr, uptr pc, uptr addr, uptr size,
                        bool write) {
  uptr begin, end;
  if (!IsValidAppMemoryRange(addr, size) ||
      !GetShadowCellRange(addr, size, &begin, &end))
    return;
  while (size) {
    const uptr chunk = Min(size, kShadowCell - addr % kShadowCell);
    MemoryAccessRange(thr, pc, addr, chunk, write);
    addr += chunk;
    size -= chunk;
  }
}

void RecordMachCopy(ThreadState *thr, uptr pc, vm_map_t target,
                    kern_return_t result, uptr source, uptr size,
                    uptr destination) {
  // The two addresses are in the target task, not necessarily our task.
  // Account only for confirmed successful local copies. This does not
  // describe partial effects of a failed call, or shared-object aliases.
  if (result != KERN_SUCCESS || target != mach_task_self() || !size)
    return;
  TrackMachCopyRange(thr, pc, source, size, false);
  TrackMachCopyRange(thr, pc, destination, size, true);
}

}  // namespace

TSAN_INTERCEPTOR(kern_return_t, mach_vm_copy, vm_map_t target,
                 mach_vm_address_t source, mach_vm_size_t size,
                 mach_vm_address_t destination) {
  SCOPED_TSAN_INTERCEPTOR(mach_vm_copy, target, source, size, destination);
  const kern_return_t result =
      REAL(mach_vm_copy)(target, source, size, destination);
  RecordMachCopy(thr, pc, target, result, source, size, destination);
  return result;
}

// The original Mach interface survives from NeXTSTEP. Use its actual SDK
// types rather than declaring it with the newer mach_vm_* signature.
TSAN_INTERCEPTOR(kern_return_t, vm_copy, vm_map_t target,
                 vm_address_t source, vm_size_t size,
                 vm_address_t destination) {
  SCOPED_TSAN_INTERCEPTOR(vm_copy, target, source, size, destination);
  const kern_return_t result = REAL(vm_copy)(target, source, size, destination);
  RecordMachCopy(thr, pc, target, result, source, size, destination);
  return result;
}
#endif  // SANITIZER_OSX

}  // namespace __tsan
