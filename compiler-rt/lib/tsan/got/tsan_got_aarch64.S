//===-- tsan_got_aarch64.S -----------------------------------------------===//
// 
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Read-only AArch64 Mach-O GOT relocation probe. Return the address of a
// TSan-owned data anchor by loading its GOT entry; do not branch through,
// modify, or authenticate dyld-owned function pointers.
//
// Mach-O prefixes C symbols with '_', so the C name beginning '__tsan'
// appears below as '___tsan'.

#if defined(__APPLE__) && defined(__aarch64__)

.text
.p2align 2
.globl ___tsan_got_aarch64_anchor_via_got
.private_extern ___tsan_got_aarch64_anchor_via_got
___tsan_got_aarch64_anchor_via_got:
  adrp x0, ___tsan_got_aarch64_anchor@GOTPAGE
  ldr  x0, [x0, ___tsan_got_aarch64_anchor@GOTPAGEOFF]
  ret

#endif  // __APPLE__ && __aarch64__
