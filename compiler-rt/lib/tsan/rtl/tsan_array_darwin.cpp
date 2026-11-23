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
#include "tsan_platform.h"
#include "tsan_rtl.h"

namespace __tsan {

static bool ContainsAddress(uptr addr, uptr begin, uptr end) {
  return begin < end && addr >= begin && addr < end;
}

// The application windows on Darwin are not necessarily contiguous. Checking
// only endpoints could miss a shadow-memory gap between them. Walk the four
// possible windows, allowing adjacent windows without crossing a gap.
static bool IsApplicationRange(uptr addr, uptr size) {
  if (size > ~(uptr)0 - addr)
    return false;
  const uptr end = addr + size;
  uptr current = addr;
  for (unsigned i = 0; i < 4 && current < end; ++i) {
    uptr covered = current;
    if (ContainsAddress(current, LoAppMemBeg(), LoAppMemEnd()))
      covered = Max(covered, LoAppMemEnd());
    if (ContainsAddress(current, MidAppMemBeg(), MidAppMemEnd()))
      covered = Max(covered, MidAppMemEnd());
    if (ContainsAddress(current, HeapMemBeg(), HeapMemEnd()))
      covered = Max(covered, HeapMemEnd());
    if (ContainsAddress(current, HiAppMemBeg(), HiAppMemEnd()))
      covered = Max(covered, HiAppMemEnd());
    if (covered == current)
      return false;
    current = covered;
  }
  return current >= end;
}

bool DarwinArrayRangeIsSupported(uptr addr, uptr size) {
  if (!size)
    return true;
  // Mach VM rounds allocations to whole pages.
  const uptr page_size = GetPageSizeCached();
  CHECK(IsPowerOfTwo(page_size));
  if (size > ~(uptr)0 - (page_size - 1))
    return false;
  return IsApplicationRange(RoundDownTo(addr, page_size),
                            RoundUpTo(size, page_size));
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
