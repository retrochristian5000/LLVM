//===-- tsan_endian_darwin.h -----------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_ENDIAN_DARWIN_H
#define TSAN_ENDIAN_DARWIN_H

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE

#include <mach-o/loader.h>

namespace __tsan {

// Mach-O byte order is encoded in the magic, not in the host CPU triple.
// These functions decode read-only header metadata; they do not load images.
enum class MachOByteOrder { kUnsupported, kNative, kSwapped };

MachOByteOrder MachO64ByteOrder(const mach_header *header);

// Decode CPU identifiers in both native and byte-swapped 64-bit Mach-O
// headers. The subtype capability flags are removed after byte swapping.
// Returns false for null pointers and unsupported Mach-O magics.
bool DecodeMachO64Architecture(const mach_header *header,
                               cpu_type_t *cpu_type,
                               cpu_subtype_t *cpu_subtype);

}  // namespace __tsan

#endif  // SANITIZER_APPLE

#endif  // TSAN_ENDIAN_DARWIN_H
