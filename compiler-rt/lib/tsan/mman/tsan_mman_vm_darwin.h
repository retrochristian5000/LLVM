//===-- tsan_mman_vm_darwin.h ---------------------------------------*- C -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_MMAN_VM_DARWIN_H
#define TSAN_MMAN_VM_DARWIN_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint64_t resident_bytes;
  uint64_t dirty_bytes;
} TsanDarwinVmUsage;

// Sample complete VM entries in [begin, end) of the current task.
// Zero means partial or invalid; valid outputs retain the sampled totals.
// A successful walk is not an atomic snapshot or a physical footprint.
__attribute__((visibility("hidden")))
int __tsan_darwin_vm_usage(uint64_t begin, uint64_t end, size_t page_size,
                          TsanDarwinVmUsage *usage);

// Read the current task using the SDK's TASK_VM_INFO declaration.
// Basic statistics are available on older revisions; bounds need revision 2.
// All sizes are bytes. Missing bounds must not be interpreted as address zero.
typedef struct {
  uint64_t virtual_bytes;
  uint64_t resident_bytes;
  uint64_t min_address;
  uint64_t max_address;
  uint32_t page_size;
  uint32_t region_count;
} TsanDarwinTaskVmInfo;

enum {
  TSAN_DARWIN_TASK_VM_UNAVAILABLE = 0,
  TSAN_DARWIN_TASK_VM_BASIC = 1,
  TSAN_DARWIN_TASK_VM_BOUNDS = 2
};

__attribute__((visibility("hidden")))
int __tsan_darwin_task_vm_info(TsanDarwinTaskVmInfo *result);

#ifdef __cplusplus
}
#endif

#endif  // TSAN_MMAN_VM_DARWIN_H
