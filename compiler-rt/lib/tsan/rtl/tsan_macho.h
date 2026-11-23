//===-- tsan_macho.h --------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_MACHO_H
#define TSAN_MACHO_H

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE
namespace __tsan {

// The main executable's Mach-O architecture; not the architecture of the
// TSan runtime dylib or of other images loaded into the process.
const char *MachOExecutableArchitecture();

}  // namespace __tsan
#endif  // SANITIZER_APPLE

#endif  // TSAN_MACHO_H
