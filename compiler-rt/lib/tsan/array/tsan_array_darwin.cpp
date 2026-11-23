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

namespace __tsan {

bool DarwinArrayRangeIsSupported(uptr addr, uptr size) {
  if (!size)
    return true;
  // Mach VM uses page-granular ranges. Round the complete [addr, addr+size)
  // interval; rounding size alone misses a second page for unaligned starts.
  const uptr page_size = GetPageSizeCached();
  CHECK(IsPowerOfTwo(page_size));
  const uptr max_addr = ~(uptr)0;
  if (size > max_addr - addr)
    return false;
  const uptr end = addr + size;
  if (end > max_addr - (page_size - 1))
    return false;
  const uptr page_begin = RoundDownTo(addr, page_size);
  const uptr page_end = RoundUpTo(end, page_size);
  return IsValidAppMemoryRange(page_begin, page_end - page_begin);
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
