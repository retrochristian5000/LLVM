//===-- tsan_interface_mmap.cpp ------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Shared POSIX mmap/munmap metadata and shadow-memory handling.
// This file does not intercept libc functions or perform mappings.
// Darwin Mach VM support remains in tsan_interceptors_mach_vm.cpp.
//
//===----------------------------------------------------------------------===//

#include "tsan_interface_mmap.h"

#if SANITIZER_POSIX && !SANITIZER_GO

#include <errno.h>

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_errno.h"
#include "../rtl/tsan_fd.h"
#include "../rtl/tsan_platform.h"
#include "../rtl/tsan_rtl.h"
#include "../warn/tsan_warn_runtime.h"

namespace __tsan {
namespace {

bool IsValidAppRange(uptr addr, uptr size) {
  // Validate the whole interval, including gaps between address windows.
  return IsValidAppMemoryRange(addr, size);
}

}  // namespace

bool PrepareMmapAddress(void **addr, uptr size, bool fixed) {
  if (!*addr || IsValidAppRange(reinterpret_cast<uptr>(*addr), size))
    return true;
  if (fixed) {
    errno = errno_EINVAL;
    return false;
  }
  *addr = nullptr;
  return true;
}

void RecordMmapResult(ThreadState *thr, uptr pc, void *requested_addr,
                      void *mapped_addr, uptr size, int fd) {
  if (!IsValidAppRange(reinterpret_cast<uptr>(mapped_addr), size)) {
    ReportUnexpectedMmapRange(reinterpret_cast<uptr>(requested_addr),
                              reinterpret_cast<uptr>(mapped_addr), size);
    Die();
  }
  // Preserve the interceptor's FD accounting convention.
  if (fd > 0)
    FdAccess(thr, pc, fd);
  MemoryRangeImitateWriteOrResetRange(thr, pc,
                                     reinterpret_cast<uptr>(mapped_addr),
                                     size);
}

void PrepareMunmapShadow(ThreadState *thr, uptr addr, uptr size) {
  UnmapShadow(thr, addr, size);
}

}  // namespace __tsan

#endif  // SANITIZER_POSIX && !SANITIZER_GO
