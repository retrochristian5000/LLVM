//===-- tsan_endian_darwin.cpp --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Mach-O header byte-order decoding for Darwin diagnostics. Generic
// little-endian data decoding remains in tsan_endian_little.cpp.
//
//===----------------------------------------------------------------------===//

#include "tsan_endian_darwin.h"

#if SANITIZER_APPLE

#include "tsan_defs.h"

namespace __tsan {

MachOByteOrder MachO64ByteOrder(const mach_header *header) {
  if (!header)
    return MachOByteOrder::kUnsupported;

  switch (header->magic) {
    case MH_MAGIC_64:
      return MachOByteOrder::kNative;
    case MH_CIGAM_64:
      return MachOByteOrder::kSwapped;
    default:
      return MachOByteOrder::kUnsupported;
  }
}

bool DecodeMachO64Architecture(const mach_header *header,
                               cpu_type_t *cpu_type,
                               cpu_subtype_t *cpu_subtype) {
  if (!cpu_type || !cpu_subtype)
    return false;

  const MachOByteOrder order = MachO64ByteOrder(header);
  if (order == MachOByteOrder::kUnsupported)
    return false;

  u32 type = static_cast<u32>(header->cputype);
  u32 subtype = static_cast<u32>(header->cpusubtype);
  if (order == MachOByteOrder::kSwapped) {
    type = __builtin_bswap32(type);
    subtype = __builtin_bswap32(subtype);
  }

  *cpu_type = static_cast<cpu_type_t>(type);
  *cpu_subtype =
      static_cast<cpu_subtype_t>(subtype & ~static_cast<u32>(CPU_SUBTYPE_MASK));
  return true;
}

}  // namespace __tsan

#endif  // SANITIZER_APPLE
