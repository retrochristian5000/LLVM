//===-- tsan_platform_darwin.cpp ------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Shared Darwin startup and Mach VM accounting. Platform-specific layout
// policy is in tsan_platform_mac.cpp and tsan_platform_ios.cpp.

#include "sanitizer_common/sanitizer_platform.h"
#if SANITIZER_APPLE

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_mac.h"
#if !SANITIZER_GO
#include "../interface/tsan_interface_got.h"
#include "tsan_macho.h"
#if defined(TSAN_HAS_OBJC_ASLR)
#include "../objc/tsan_objc_aslr.h"
#endif
#endif
#include "sanitizer_common/sanitizer_stackdepot.h"
#include "tsan_platform.h"
#include "tsan_rtl.h"
#include "../warn/tsan_warn_darwin.h"

#include <mach/mach.h>
#include <mach/vm_statistics.h>
#if defined(TSAN_HAS_NATIVE_DARWIN_VM_C)
#include "../mman/tsan_mman_vm_darwin.h"
#endif

namespace __tsan {

void InitializePlatformEarly() {
#if !SANITIZER_GO
#if defined(TSAN_HAS_NATIVE_DARWIN_VM_C)
  TsanDarwinTaskVmInfo vm = {};
  const int status = __tsan_darwin_task_vm_info(&vm);
  const uptr kernel_limit =
      status == TSAN_DARWIN_TASK_VM_BOUNDS ? vm.max_address : 0;
  // The kernel's address ceiling and the sanitizer's table ceiling are
  // separate. A missing revision retains the desktop conservative fallback.
  const uptr supported_limit =
      kernel_limit ? Min<uptr>(kernel_limit, SANITIZER_MMAP_RANGE_SIZE)
                   : SANITIZER_MMAP_RANGE_SIZE;
  if (status != TSAN_DARWIN_TASK_VM_UNAVAILABLE) {
    VPrintf(2,
            "ThreadSanitizer: native Darwin task VM: virtual=%zu bytes "
            "resident=%zu bytes regions=%u page_size=%u bounds=%s\n",
            static_cast<uptr>(vm.virtual_bytes),
            static_cast<uptr>(vm.resident_bytes), vm.region_count, vm.page_size,
            kernel_limit ? "available" : "unavailable");
    if (vm.page_size != GetPageSizeCached())
      VPrintf(1, "ThreadSanitizer: Darwin task VM page size differs from "
                 "cached native page size\n");
  } else {
    VPrintf(1, "ThreadSanitizer: native Darwin task VM query unavailable\n");
  }
#else
  const uptr kernel_limit = GetDarwinTaskVmMaxAddress();
  const uptr supported_limit = GetMaxUserVirtualAddress() + 1;
#endif
  VPrintf(1,
          "ThreadSanitizer: Darwin VM limit: kernel=%p supported=%p "
          "layout_end=%p\n",
          reinterpret_cast<void *>(kernel_limit),
          reinterpret_cast<void *>(supported_limit),
          reinterpret_cast<void *>(HiAppMemEnd()));
  ValidateDarwinPlatformLayout(supported_limit);
#endif
}

void InitializePlatform() {
  DisableCoreDumperIfNecessary();
#if !SANITIZER_GO
  VPrintf(2, "ThreadSanitizer: main Mach-O executable architecture: %s\n",
          MachOExecutableArchitecture());
  if (Verbosity() >= 2) {
#if defined(TSAN_HAS_OBJC_ASLR)
    ReportObjCImagePlacement();
#endif
    MachOGOTSlots slots;
    if (GetMainExecutableMachOGOTSlots(&slots))
      VPrintf(2,
              "ThreadSanitizer: main Mach-O symbol-pointer slots: "
              "lazy=%zu, non-lazy=%zu\n",
              slots.lazy, slots.non_lazy);
#if defined(__aarch64__) && !SANITIZER_GO
    VPrintf(2, "ThreadSanitizer: AArch64 Mach-O GOT data relocation: %s\n",
            VerifyAArch64GOTAnchor() ? "resolved" : "mismatch");
#endif
  }
  if (!CheckAndProtect(true, true, true)) {
    ReportDarwinIncompatibleMemoryLayout();
    Die();
  }

  InitializeDarwinThreadSupport();
#endif

  InitializeDarwinLongJmp();
}

#if !SANITIZER_GO
void InitializeShadowMemoryPlatform() {}


bool ReserveDarwinVmRange(uptr begin, uptr end, bool accessible,
                          const char *name) {
  const uptr page_size = GetPageSizeCached();
  CHECK_LE(begin, end);
  CHECK(IsAligned(begin, page_size));
  CHECK(IsAligned(end, page_size));
  if (begin == end)
    return true;

  // Unlike mmap(MAP_FIXED), a fixed Mach mapping without OVERWRITE fails
  // on an occupied range. This also protects anonymous mappings omitted
  // by the Mach-O image iterator.
  vm_address_t address = begin;
  const vm_prot_t protection =
      accessible ? VM_PROT_READ | VM_PROT_WRITE : VM_PROT_NONE;
  const kern_return_t kr =
      vm_map(mach_task_self(), &address, end - begin, 0,
             VM_FLAGS_FIXED | VM_MAKE_TAG(VM_MEMORY_SANITIZER), MACH_PORT_NULL,
             0, FALSE, protection, VM_PROT_READ | VM_PROT_WRITE,
             VM_INHERIT_COPY);
  if (kr != KERN_SUCCESS) {
    ReportDarwinVmReservationFailure(name, begin, end, kr);
    return false;
  }
  CHECK_EQ(address, begin);
  if (accessible)
    IncreaseTotalMmap(end - begin);
  return true;
}
#endif

// Region totals cannot be attributed exactly to a subrange of a Mach entry.
// Count only complete entries and explicitly mark incomplete samples.
static bool RegionMemUsage(uptr start, uptr end, uptr *res, uptr *dirty) {
#if defined(TSAN_HAS_NATIVE_DARWIN_VM_C)
  TsanDarwinVmUsage usage = {};
  const bool complete =
      __tsan_darwin_vm_usage(start, end, GetPageSizeCached(), &usage);
  static_assert(sizeof(uptr) == sizeof(uint64_t), "native Darwin VM width");
  *res = usage.resident_bytes;
  *dirty = usage.dirty_bytes;
  return complete;
#else
  vm_address_t address = start;
  const uptr page_size = GetPageSizeCached();
  uptr resident_pages = 0;
  uptr dirty_pages = 0;
  bool complete = true;
  while (address < end) {
    const vm_address_t cursor = address;
    vm_size_t size = 0;
    mach_msg_type_number_t count = VM_REGION_EXTENDED_INFO_COUNT;
    vm_region_extended_info_data_t info = {};
    mach_port_t object_name = MACH_PORT_NULL;
    const kern_return_t kr = vm_region_64(
        mach_task_self(), &address, &size, VM_REGION_EXTENDED_INFO,
        reinterpret_cast<vm_region_info_t>(&info), &count, &object_name);
    if (MACH_PORT_VALID(object_name))
      mach_port_deallocate(mach_task_self(), object_name);
    if (kr == KERN_INVALID_ADDRESS)
      break;  // No further mapped regions.
    if (kr != KERN_SUCCESS) {
      complete = false;
      break;
    }
    if (address >= end)
      break;  // The next mapping belongs to another window.
    if (!size || size > ~(uptr)0 - address || address + size <= cursor ||
        count < VM_REGION_EXTENDED_INFO_COUNT) {
      complete = false;
      break;
    }
    const uptr next = address + size;
    if (address < start || next > end) {
      complete = false;
    } else {
      const uptr max_pages = ~(uptr)0 / page_size;
      if (info.pages_resident > max_pages - resident_pages ||
          info.pages_dirtied > max_pages - dirty_pages) {
        complete = false;
        break;
      }
      resident_pages += info.pages_resident;
      dirty_pages += info.pages_dirtied;
    }
    address = next;
  }
  *res = resident_pages * page_size;
  *dirty = dirty_pages * page_size;
  return complete;
#endif
}

void WriteMemoryProfile(char *buf, uptr buf_size, u64 uptime_ns) {
  uptr shadow_res, shadow_dirty;
  uptr meta_res, meta_dirty;
  bool complete =
      RegionMemUsage(ShadowBeg(), ShadowEnd(), &shadow_res, &shadow_dirty);
  complete &= RegionMemUsage(MetaShadowBeg(), MetaShadowEnd(),
                             &meta_res, &meta_dirty);

#  if !SANITIZER_GO
  uptr low_res, low_dirty;
  uptr mid_res, mid_dirty;
  uptr high_res, high_dirty;
  uptr heap_res, heap_dirty;
  complete &= RegionMemUsage(LoAppMemBeg(), LoAppMemEnd(), &low_res, &low_dirty);
  complete &= RegionMemUsage(MidAppMemBeg(), MidAppMemEnd(), &mid_res, &mid_dirty);
  complete &= RegionMemUsage(HiAppMemBeg(), HiAppMemEnd(), &high_res, &high_dirty);
  complete &= RegionMemUsage(HeapMemBeg(), HeapMemEnd(), &heap_res, &heap_dirty);
#else  // !SANITIZER_GO
  uptr app_res, app_dirty;
  complete &= RegionMemUsage(LoAppMemBeg(), LoAppMemEnd(), &app_res, &app_dirty);
#endif

  StackDepotStats stacks = StackDepotGetStats();
  uptr nthread, nlive;
  ctx->thread_registry.GetNumberOfThreads(&nthread, &nlive);
  internal_snprintf(
      buf, buf_size,
      "shadow   (0x%016zx-0x%016zx): resident %zd kB, dirty %zd kB\n"
      "meta     (0x%016zx-0x%016zx): resident %zd kB, dirty %zd kB\n"
#  if !SANITIZER_GO
      "low app  (0x%016zx-0x%016zx): resident %zd kB, dirty %zd kB\n"
      "mid app  (0x%016zx-0x%016zx): resident %zd kB, dirty %zd kB\n"
      "high app (0x%016zx-0x%016zx): resident %zd kB, dirty %zd kB\n"
      "heap     (0x%016zx-0x%016zx): resident %zd kB, dirty %zd kB\n"
#  else  // !SANITIZER_GO
      "app      (0x%016zx-0x%016zx): resident %zd kB, dirty %zd kB\n"
#  endif
      "VM region accounting: %s\n"
      "stacks: %zd unique IDs, %zd kB allocated\n"
      "threads: %zd total, %zd live\n"
      "------------------------------\n",
      ShadowBeg(), ShadowEnd(), shadow_res / 1024, shadow_dirty / 1024,
      MetaShadowBeg(), MetaShadowEnd(), meta_res / 1024, meta_dirty / 1024,
#  if !SANITIZER_GO
      LoAppMemBeg(), LoAppMemEnd(), low_res / 1024, low_dirty / 1024,
      MidAppMemBeg(), MidAppMemEnd(), mid_res / 1024, mid_dirty / 1024,
      HiAppMemBeg(), HiAppMemEnd(), high_res / 1024, high_dirty / 1024,
      HeapMemBeg(), HeapMemEnd(), heap_res / 1024, heap_dirty / 1024,
#  else  // !SANITIZER_GO
      LoAppMemBeg(), LoAppMemEnd(), app_res / 1024, app_dirty / 1024,
#  endif
      complete ? "complete entries" : "partial (unattributed regions omitted)",
      stacks.n_uniq_ids, stacks.allocated / 1024, nthread, nlive);
}
}  // namespace __tsan

#endif  // SANITIZER_APPLE
