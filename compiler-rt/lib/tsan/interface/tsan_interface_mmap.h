//===-- tsan_interface_mmap.h ---------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Internal POSIX mmap/munmap hooks used by the interceptor layer.
// The libc calls and their platform-specific signatures remain in rtl/.
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_INTERFACE_MMAP_H
#define TSAN_INTERFACE_MMAP_H

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_POSIX && !SANITIZER_GO

#include "sanitizer_common/sanitizer_internal_defs.h"

namespace __tsan {

struct ThreadState;

// Validate an mmap address hint. Invalid non-fixed hints are cleared;
// invalid fixed mappings fail with EINVAL.
bool PrepareMmapAddress(void **addr, uptr size, bool fixed);

// Called after successful mmap; validates the mapped range and records
// FD and shadow-memory metadata without changing the mapped contents.
void RecordMmapResult(ThreadState *thr, uptr pc, void *requested_addr,
                      void *mapped_addr, uptr size, int fd);

// Match the existing unmap interceptor ordering: shadow state is updated
// before calling the platform munmap implementation.
void PrepareMunmapShadow(ThreadState *thr, uptr addr, uptr size);

}  // namespace __tsan

#endif  // SANITIZER_POSIX && !SANITIZER_GO

#endif  // TSAN_INTERFACE_MMAP_H
