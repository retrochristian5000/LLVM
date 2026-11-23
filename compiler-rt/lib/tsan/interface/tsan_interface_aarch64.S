//===-- tsan_interface_aarch64.S ------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Internal AArch64 ABI helper for normalizing instrumentation caller PCs.
// XPACLRI removes an instruction-address PAC only when branch signing is
// enabled. This does not authenticate the pointer or alter its code address.
// Save and restore x30: it holds the return address for this ABI boundary.
// x0: code address; result is the normalized code address in x0.

#if defined(__aarch64__)
.text
.p2align 2

#if defined(__APPLE__)
.globl ___tsan_interface_aarch64_strip_pc
.private_extern ___tsan_interface_aarch64_strip_pc
___tsan_interface_aarch64_strip_pc:
#else
.globl __tsan_interface_aarch64_strip_pc
.hidden __tsan_interface_aarch64_strip_pc
.type __tsan_interface_aarch64_strip_pc, %function
__tsan_interface_aarch64_strip_pc:
#endif
  bti c
#if defined(__arm64e__) || defined(__ARM_FEATURE_PAC_DEFAULT)
  // XPACLRI operates on x30, not directly on the argument in x0.
  mov x9, x30
  mov x30, x0
  hint #7
  mov x0, x30
  mov x30, x9
#endif
  ret

#if !defined(__APPLE__)
.size __tsan_interface_aarch64_strip_pc, .-__tsan_interface_aarch64_strip_pc
.section .note.GNU-stack,"",%progbits
#endif
#endif  // __aarch64__
