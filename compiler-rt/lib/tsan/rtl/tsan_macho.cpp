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
  if (!header || header->magic != MH_MAGIC_64)
    return "unknown";

  if (header->cputype == CPU_TYPE_X86_64)
    return "x86_64";

  if (header->cputype == CPU_TYPE_ARM64) {
    // CPU_SUBTYPE_MASK contains optional capability flags, not the subtype.
    const cpu_subtype_t subtype = header->cpusubtype & ~CPU_SUBTYPE_MASK;
    return subtype == CPU_SUBTYPE_ARM64E ? "arm64e" : "arm64";
  }

  return "unknown";
}

}  // namespace __tsan

#endif  // SANITIZER_APPLE
