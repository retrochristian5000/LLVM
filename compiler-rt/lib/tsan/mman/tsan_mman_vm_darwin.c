//===-- tsan_mman_vm_darwin.c --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tsan_mman_vm_darwin.h"

#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/vm_region.h>

_Static_assert(sizeof(mach_vm_address_t) == sizeof(uint64_t),
               "Mach VM address width");
_Static_assert(sizeof(mach_vm_size_t) == sizeof(uint64_t),
               "Mach VM size width");

int __tsan_darwin_vm_usage(uint64_t begin, uint64_t end, size_t page_size,
                          TsanDarwinVmUsage *usage) {
  if (!usage)
    return 0;
  *usage = (TsanDarwinVmUsage){0, 0};
  if (begin > end || !page_size || (page_size & (page_size - 1)))
    return 0;

  const mach_port_t task = mach_task_self();
  mach_vm_address_t cursor = begin;
  int complete = 1;
  const uint64_t max_pages = UINT64_MAX / page_size;
  uint64_t resident = 0, dirty = 0;
  while (cursor < end) {
    mach_vm_address_t address = cursor;
    mach_vm_size_t size = 0;
    vm_region_extended_info_data_t info = {0};
    mach_msg_type_number_t count = VM_REGION_EXTENDED_INFO_COUNT;
    mach_port_t object = MACH_PORT_NULL;
    const kern_return_t kr =
        mach_vm_region(task, &address, &size, VM_REGION_EXTENDED_INFO,
                       (vm_region_info_t)&info, &count, &object);
    if (MACH_PORT_VALID(object))
      mach_port_deallocate(task, object);
    if (kr == KERN_INVALID_ADDRESS)
      break;
    if (kr != KERN_SUCCESS) {
      complete = 0;
      break;
    }
    if (!size || size > UINT64_MAX - address || address + size <= cursor ||
        count != VM_REGION_EXTENDED_INFO_COUNT) {
      complete = 0;
      break;
    }
    if (address >= end)
      break;

    const mach_vm_address_t next = address + size;
    // Region statistics describe the entire entry. Never prorate or count
    // an overlapping entry twice, including one changed during the walk.
    if (address < cursor || next > end) {
      complete = 0;
    } else {
      if (info.pages_resident > max_pages - resident ||
          info.pages_dirtied > max_pages - dirty) {
        complete = 0;
        break;
      }
      resident += info.pages_resident;
      dirty += info.pages_dirtied;
    }
    cursor = next;
  }
  usage->resident_bytes = resident * page_size;
  usage->dirty_bytes = dirty * page_size;
  return complete;
}
