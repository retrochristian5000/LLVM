//===-- tsan_mman_range_posix.h --------------------------------------*- C -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_MMAN_RANGE_POSIX_H
#define TSAN_MMAN_RANGE_POSIX_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compute the whole-page extent of a nonempty byte range. Return zero
// without modifying outputs on invalid geometry or integer overflow.
// This is address arithmetic only, not a statement about VM permissions.
__attribute__((visibility("hidden")))
int __tsan_posix_page_extent(uintptr_t addr, size_t size, size_t page_size,
                            uintptr_t *begin, size_t *length);

#ifdef __cplusplus
}
#endif

#endif  // TSAN_MMAN_RANGE_POSIX_H
