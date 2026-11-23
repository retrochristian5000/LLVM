//===-- tsan_endian.h -------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Byte-order conversion for data formats with a fixed on-disk byte order.
// Shadow-memory bitfield layout is defined separately in tsan_shadow.h.

#ifndef TSAN_ENDIAN_H
#define TSAN_ENDIAN_H

#include "../rtl/tsan_defs.h"

namespace __tsan {

// Decode count little-endian 32-bit words from potentially unaligned bytes.
// The destination must hold at least count words and must not overlap src.
void ReadLittleEndian32Words(u32 *dst, const void *src, uptr count);

}  // namespace __tsan

#endif  // TSAN_ENDIAN_H
