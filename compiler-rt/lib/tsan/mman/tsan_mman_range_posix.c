//===-- tsan_mman_range_posix.c -------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tsan_mman_range_posix.h"

int __tsan_posix_page_extent(uintptr_t addr, size_t size, size_t page_size,
                            uintptr_t *begin, size_t *length) {
  if (!begin || !length || !size || !page_size ||
      (page_size & (page_size - 1)) || page_size > UINTPTR_MAX ||
      size > UINTPTR_MAX - addr)
    return 0;

  const uintptr_t mask = (uintptr_t)page_size - 1;
  const uintptr_t first = addr & ~mask;
  uintptr_t end = addr + (uintptr_t)size;
  const uintptr_t remainder = end & mask;
  if (remainder) {
    const uintptr_t padding = (uintptr_t)page_size - remainder;
    if (padding > UINTPTR_MAX - end)
      return 0;
    end += padding;
  }
  if (end - first > SIZE_MAX)
    return 0;

  *begin = first;
  *length = (size_t)(end - first);
  return 1;
}
