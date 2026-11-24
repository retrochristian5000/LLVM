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
#if defined(TSAN_HAS_NATIVE_POSIX_C)
#include "../mman/tsan_mman_range_posix.h"
#endif

namespace __tsan {
namespace {

bool IsValidAppRange(uptr addr, uptr size) {
  // Validate the whole interval, including gaps between address windows.
  return IsValidAppMemoryRange(addr, size);
}

#if defined(TSAN_HAS_NATIVE_POSIX_C)
// Only the C frontend calculates the covered pages. The TSan mapping
// policy remains here; the C helper neither maps nor reads user memory.
bool GetNativePosixRange(uptr addr, uptr size, uptr *begin, uptr *length) {
  uintptr_t page_begin;
  size_t page_length;
  if (!__tsan_posix_page_extent(addr, size, GetPageSizeCached(),
                               &page_begin, &page_length) ||
      !IsValidAppRange(page_begin, page_length))
    return false;
  *begin = page_begin;
  *length = page_length;
  return true;
}
#endif

}  // namespace

bool PrepareMmapAddress(void **addr, uptr size, bool fixed) {
#if defined(TSAN_HAS_NATIVE_POSIX_C)
  if (fixed) {
    uptr begin, length;
    if (GetNativePosixRange(reinterpret_cast<uptr>(*addr), size, &begin,
                            &length) &&
        begin == reinterpret_cast<uptr>(*addr))
      return true;
    errno = errno_EINVAL;
    return false;
  }
#endif
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
#if defined(TSAN_HAS_NATIVE_POSIX_C)
  uptr begin, length;
  const bool valid =
      GetNativePosixRange(reinterpret_cast<uptr>(mapped_addr), size, &begin,
                          &length);
#else
  const bool valid = IsValidAppRange(reinterpret_cast<uptr>(mapped_addr), size);
#endif
  if (!valid) {
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
  uptr rounded_size;
#if defined(TSAN_HAS_NATIVE_POSIX_C)
  uptr begin;
  if (GetNativePosixRange(addr, size, &begin, &rounded_size) && begin == addr)
#else
  if (GetPosixMunmapRange(addr, size, GetPageSizeCached(), &rounded_size))
#endif
    UnmapShadow(thr, addr, rounded_size);
}

}  // namespace __tsan

#endif  // SANITIZER_POSIX && !SANITIZER_GO
