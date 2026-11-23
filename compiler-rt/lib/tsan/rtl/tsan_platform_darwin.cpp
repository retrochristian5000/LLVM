//===-- tsan_platform_darwin.cpp ------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Darwin Mach VM accounting and libc jump-buffer ABI. These interfaces are
// shared across Darwin platforms, not specific to the macOS application layer.

#include "sanitizer_common/sanitizer_platform.h"
#if SANITIZER_APPLE

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_stackdepot.h"
#include "tsan_platform.h"
#include "tsan_rtl.h"

#include <mach/mach.h>

namespace __tsan {

static void RegionMemUsage(uptr start, uptr end, uptr *res, uptr *dirty) {
  vm_address_t address = start;
  vm_address_t end_address = end;
  uptr resident_pages = 0;
  uptr dirty_pages = 0;
  while (address < end_address) {
    vm_size_t vm_region_size;
    mach_msg_type_number_t count = VM_REGION_EXTENDED_INFO_COUNT;
    vm_region_extended_info_data_t vm_region_info;
    mach_port_t object_name;
    kern_return_t ret = vm_region_64(
        mach_task_self(), &address, &vm_region_size, VM_REGION_EXTENDED_INFO,
        (vm_region_info_t)&vm_region_info, &count, &object_name);
    if (ret != KERN_SUCCESS) break;

    resident_pages += vm_region_info.pages_resident;
    dirty_pages += vm_region_info.pages_dirtied;

    address += vm_region_size;
  }
  *res = resident_pages * GetPageSizeCached();
  *dirty = dirty_pages * GetPageSizeCached();
}

void WriteMemoryProfile(char *buf, uptr buf_size, u64 uptime_ns) {
  uptr shadow_res, shadow_dirty;
  uptr meta_res, meta_dirty;
  RegionMemUsage(ShadowBeg(), ShadowEnd(), &shadow_res, &shadow_dirty);
  RegionMemUsage(MetaShadowBeg(), MetaShadowEnd(), &meta_res, &meta_dirty);

#  if !SANITIZER_GO
  uptr low_res, low_dirty;
  uptr high_res, high_dirty;
  uptr heap_res, heap_dirty;
  RegionMemUsage(LoAppMemBeg(), LoAppMemEnd(), &low_res, &low_dirty);
  RegionMemUsage(HiAppMemBeg(), HiAppMemEnd(), &high_res, &high_dirty);
  RegionMemUsage(HeapMemBeg(), HeapMemEnd(), &heap_res, &heap_dirty);
#else  // !SANITIZER_GO
  uptr app_res, app_dirty;
  RegionMemUsage(LoAppMemBeg(), LoAppMemEnd(), &app_res, &app_dirty);
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
      "high app (0x%016zx-0x%016zx): resident %zd kB, dirty %zd kB\n"
      "heap     (0x%016zx-0x%016zx): resident %zd kB, dirty %zd kB\n"
#  else  // !SANITIZER_GO
      "app      (0x%016zx-0x%016zx): resident %zd kB, dirty %zd kB\n"
#  endif
      "stacks: %zd unique IDs, %zd kB allocated\n"
      "threads: %zd total, %zd live\n"
      "------------------------------\n",
      ShadowBeg(), ShadowEnd(), shadow_res / 1024, shadow_dirty / 1024,
      MetaShadowBeg(), MetaShadowEnd(), meta_res / 1024, meta_dirty / 1024,
#  if !SANITIZER_GO
      LoAppMemBeg(), LoAppMemEnd(), low_res / 1024, low_dirty / 1024,
      HiAppMemBeg(), HiAppMemEnd(), high_res / 1024, high_dirty / 1024,
      HeapMemBeg(), HeapMemEnd(), heap_res / 1024, heap_dirty / 1024,
#  else  // !SANITIZER_GO
      LoAppMemBeg(), LoAppMemEnd(), app_res / 1024, app_dirty / 1024,
#  endif
      stacks.n_uniq_ids, stacks.allocated / 1024, nthread, nlive);
}
}  // namespace __tsan

#endif  // SANITIZER_APPLE
