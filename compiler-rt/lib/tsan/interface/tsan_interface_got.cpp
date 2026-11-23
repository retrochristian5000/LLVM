//===-- tsan_interface_got.cpp ------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// AArch64 Mach-O GOT relocation check for TSan diagnostics.
// Darwin's read-only Mach-O pointer inventory is in got/tsan_got_darwin.cpp.
// Never rewrite dyld-owned or authenticated arm64e pointers.

#include "tsan_interface_got.h"

#if SANITIZER_APPLE


namespace __tsan {


#if defined(__aarch64__)
// A data anchor exercises a Mach-O AArch64 GOT relocation without reading
// or changing any dyld-owned symbol pointer or ARM64e function pointer.
extern "C" const unsigned char __tsan_got_aarch64_anchor = 0;
extern "C" const unsigned char *__tsan_got_aarch64_anchor_via_got();

bool VerifyAArch64GOTAnchor() {
  return __tsan_got_aarch64_anchor_via_got() == &__tsan_got_aarch64_anchor;
}
#endif

}  // namespace __tsan

#endif  // SANITIZER_APPLE
