//===-- tsan_mman_posix.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// POSIX allocator alignment semantics shared by TSan's libc interceptors.
// Keep the platform-independent allocator and other allocation functions in
// tsan_mman.cpp; Darwin malloc-zone hooks live in tsan_mman_mac.cpp.
//
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_POSIX && !SANITIZER_GO

#include "tsan_mman.h"

#include "sanitizer_common/sanitizer_allocator_checks.h"
#include "sanitizer_common/sanitizer_allocator_interface.h"
#include "sanitizer_common/sanitizer_allocator_report.h"
#include "sanitizer_common/sanitizer_errno.h"
#include "../rtl/tsan_rtl.h"

namespace __tsan {

int user_posix_memalign(ThreadState *thr, uptr pc, void **memptr, uptr align,
                        uptr sz) {
  if (UNLIKELY(!CheckPosixMemalignAlignment(align))) {
    if (AllocatorMayReturnNull())
      return errno_EINVAL;
    GET_STACK_TRACE_FATAL(thr, pc);
    ReportInvalidPosixMemalignAlignment(align, &stack);
  }
  void *ptr = user_alloc_internal(thr, pc, sz, align);
  if (UNLIKELY(!ptr))
    // OOM error is already taken care of by user_alloc_internal.
    return errno_ENOMEM;
  CHECK(IsAligned((uptr)ptr, align));
  *memptr = ptr;
  return 0;
}

}  // namespace __tsan

#endif  // SANITIZER_POSIX && !SANITIZER_GO
