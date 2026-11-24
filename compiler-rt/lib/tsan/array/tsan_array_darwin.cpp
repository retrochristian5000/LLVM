//===-- tsan_array_darwin.cpp --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Darwin-specific range validation and TLS shadow initialization.
// Generic bulk array shadow operations remain in tsan_array.cpp.
//
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE && !SANITIZER_GO

#include "sanitizer_common/sanitizer_common.h"
#include "tsan_array_darwin.h"
#include "../rtl/tsan_platform.h"
#include "../rtl/tsan_rtl.h"
#if defined(TSAN_HAS_NATIVE_POSIX_C)
#include "../mman/tsan_mman_range_posix.h"
#endif

namespace __tsan {

DarwinArrayRangeDiagnosis DiagnoseDarwinArrayRange(uptr addr, uptr size) {
  DarwinArrayRangeDiagnosis result = {AppMemoryRangeError::kNone, 0, 0, 0};
  if (!size)
    return result;

  const uptr page_size = GetPageSizeCached();
  CHECK(IsPowerOfTwo(page_size));
#if defined(TSAN_HAS_NATIVE_POSIX_C)
  // The arithmetic is shared with POSIX, but Mach deallocation accepts
  // an unaligned start. Do not import munmap's alignment requirement.
  uintptr_t begin;
  size_t length;
  if (!__tsan_posix_page_extent(addr, size, page_size, &begin, &length)) {
    result.error = AppMemoryRangeError::kOverflow;
    return result;
  }
  result.page_begin = begin;
  result.page_end = begin + length;
#else
  const uptr max_addr = ~(uptr)0;
  if (size > max_addr - addr) {
    result.error = AppMemoryRangeError::kOverflow;
    return result;
  }
  const uptr end = addr + size;
  if (end > max_addr - (page_size - 1)) {
    result.error = AppMemoryRangeError::kOverflow;
    return result;
  }
  result.page_begin = RoundDownTo(addr, page_size);
  result.page_end = RoundUpTo(end, page_size);
#endif
  result.error = DiagnoseAppMemoryRange(
      result.page_begin, result.page_end - result.page_begin,
      &result.first_unsupported);
  return result;
}

bool DarwinArrayRangeIsSupported(uptr addr, uptr size) {
  return DiagnoseDarwinArrayRange(addr, size).error == AppMemoryRangeError::kNone;
}

DarwinArrayRangeDiagnosis DiagnoseDarwinAllocationRange(uptr addr, uptr size) {
  const uptr page_size = GetPageSizeCached();
  CHECK(IsPowerOfTwo(page_size));
  return DiagnoseDarwinArrayRange(RoundDownTo(addr, page_size), size);
}

extern "C" void __tsan_tls_initialization() {}

void ImitateTlsWrite(ThreadState *thr, uptr tls_addr, uptr tls_size) {
  if (!tls_size)
    return;

  const uptr pc = StackTrace::GetNextInstructionPc(
      reinterpret_cast<uptr>(__tsan_tls_initialization));
  // Unlike Linux, we only store a pointer to the ThreadState object in TLS;
  // just mark the entire range as written to.
  MemoryRangeImitateWrite(thr, pc, tls_addr, tls_size);
}

}  // namespace __tsan

#endif  // SANITIZER_APPLE && !SANITIZER_GO
