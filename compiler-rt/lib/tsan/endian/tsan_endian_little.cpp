//===-- tsan_endian_little.cpp --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Fast native-endian decoding for little-endian CPUs (x86_64, AArch64,
// ARM64e, and little-endian PowerPC64). The generic implementation for
// big-endian hosts and unknown byte orders is in tsan_endian.cpp.

#include "tsan_endian.h"

#include "sanitizer_common/sanitizer_libc.h"

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
    __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

namespace __tsan {

void ReadLittleEndian32Words(u32 *dst, const void *src, uptr count) {
  // A byte copy preserves the native little-endian word representation and
  // also handles unaligned input without strict-aliasing violations.
  internal_memcpy(dst, src, count * sizeof(*dst));
}

}  // namespace __tsan

#endif  // known little-endian host
