//===-- tsan_macho.cpp ----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// TSan-specific Mach-O diagnostics. General Mach-O segment, UUID, and image
// enumeration belongs to sanitizer_common/sanitizer_procmaps_mac.cpp.
// Mach VM accounting and Darwin jump-buffer ABI handling belong elsewhere.

#include "tsan_macho.h"
#include "tsan_endian_darwin.h"

#if SANITIZER_APPLE

#include <mach/machine.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>

namespace __tsan {

const mach_header *MachOExecutableHeader() {
  return _dyld_get_image_header(0);
}

const char *MachOExecutableArchitecture() {
  const mach_header *header = MachOExecutableHeader();
  cpu_type_t cpu_type;
  cpu_subtype_t cpu_subtype;
  if (!DecodeMachO64Architecture(header, &cpu_type, &cpu_subtype))
    return "unknown";

  if (cpu_type == CPU_TYPE_X86_64)
    return "x86_64";

  if (cpu_type == CPU_TYPE_ARM64)
    return cpu_subtype == CPU_SUBTYPE_ARM64E ? "arm64e" : "arm64";

  return "unknown";
}

}  // namespace __tsan

#endif  // SANITIZER_APPLE
