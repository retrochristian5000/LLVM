//===-- tsan_objc_darwin.mm -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE && !SANITIZER_GO
#  include <objc/objc.h>
#  include <stddef.h>

#  include "../rtl/tsan_interceptors.h"
#  include "../rtl/tsan_platform.h"
#  include "interception/interception.h"

namespace __tsan {
namespace {

void TrackStructRange(ThreadState *thr, uptr pc, const void *ptr, uptr size,
                      bool write) {
  uptr addr = reinterpret_cast<uptr>(ptr);
  uptr begin, end;
  if (!ptr || !IsValidAppMemoryRange(addr, size) ||
      !GetShadowCellRange(addr, size, &begin, &end))
    return;

  // Copy exact bytes, not the full shadow-cell width. Recompute the mapping
  // at cell boundaries rather than assuming contiguous shadow addresses.
  while (size) {
    const uptr chunk = Min(size, kShadowCell - addr % kShadowCell);
    MemoryAccessRange(thr, pc, addr, chunk, write);
    addr += chunk;
    size -= chunk;
  }
}

}  // namespace

// The native BOOL ABI differs between Darwin architectures. Compile this
// bridge as Objective-C++ and let the SDK supply the declaration's types.
TSAN_INTERCEPTOR(void, objc_copyStruct, void *dest, const void *src,
                 ptrdiff_t size, BOOL atomic, BOOL hasStrong) {
  // libobjc's atomic branch uses locks for both source and destination.
  // Tracking outside those locks would invent a race; leave it untouched.
  // The runtime also owns the semantics of invalid sizes and strong fields.
  if (atomic || size <= 0)
    return REAL(objc_copyStruct)(dest, src, size, atomic, hasStrong);

  SCOPED_TSAN_INTERCEPTOR(objc_copyStruct, dest, src, size, atomic, hasStrong);
  const uptr bytes = static_cast<uptr>(size);
  TrackStructRange(thr, pc, src, bytes, false);
  TrackStructRange(thr, pc, dest, bytes, true);
  REAL(objc_copyStruct)(dest, src, size, atomic, hasStrong);
}

}  // namespace __tsan

#endif  // SANITIZER_APPLE && !SANITIZER_GO
