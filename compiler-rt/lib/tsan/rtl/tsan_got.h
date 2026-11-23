//===-- tsan_got.h ----------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_GOT_H
#define TSAN_GOT_H

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE

#include "sanitizer_common/sanitizer_internal_defs.h"

namespace __tsan {

// Mach-O's symbol-pointer sections serve a role similar to an ELF GOT.
// This is read-only metadata; dyld owns all binding and pointer signing.
struct MachOGOTSlots {
  __sanitizer::uptr lazy;
  __sanitizer::uptr non_lazy;
};

// Returns false for a missing, unsupported, or malformed Mach-O header.
// The counts are only meaningful on success.
bool GetMainExecutableMachOGOTSlots(MachOGOTSlots *slots);

}  // namespace __tsan

#endif  // SANITIZER_APPLE
#endif  // TSAN_GOT_H
