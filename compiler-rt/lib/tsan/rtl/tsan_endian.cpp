//===-- tsan_endian.cpp --------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Decode fixed little-endian data independently of host byte order or input
// alignment. MD5 uses this to read message words on PPC64/s390x big-endian
// hosts without relying on unaligned or aliasing 32-bit pointer casts.

#include "tsan_endian.h"

#include "sanitizer_common/sanitizer_libc.h"

namespace __tsan {

void ReadLittleEndian32Words(u32 *dst, const void *src, uptr count) {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  // Word values already match their representation in memory.
  internal_memcpy(dst, src, count * sizeof(*dst));
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  // Copy first: source alignment is not guaranteed by the caller.
  internal_memcpy(dst, src, count * sizeof(*dst));
  for (uptr i = 0; i < count; ++i)
    dst[i] = __builtin_bswap32(dst[i]);
#else
  // Conservative fallback if a supported compiler does not expose endian
  // predefines. Never rely on the alignment of user-supplied input.
  const u8 *bytes = static_cast<const u8 *>(src);
  for (uptr i = 0; i < count; ++i) {
    const uptr off = i * sizeof(*dst);
    dst[i] = static_cast<u32>(bytes[off]) |
             (static_cast<u32>(bytes[off + 1]) << 8) |
             (static_cast<u32>(bytes[off + 2]) << 16) |
             (static_cast<u32>(bytes[off + 3]) << 24);
  }
#endif
}

}  // namespace __tsan
