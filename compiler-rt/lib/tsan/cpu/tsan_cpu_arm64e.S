//===-- tsan_cpu_arm64e.S ------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Darwin arm64e Data-B pointer authentication for libc jump-buffer SPs.
// x0: signed stack pointer, x1: Clang's "sp" string discriminator.
// Return x0: authenticated stack pointer. This leaf function does not modify
// SP/LR or saved registers and must never turn failed authentication into an
// unauthenticated pointer.
//
// Replicate Clang's ptrauth_auth_data(..., ptrauth_key_asdb, discriminator)
// lowering, including the XPACD comparison and authentication failure trap.
// The caller handles the OS-specific jump-buffer layout and XOR decoding.
//
//===----------------------------------------------------------------------===//

#if defined(__APPLE__) && defined(__arm64e__)
.section __TEXT,__text,regular,pure_instructions
.p2align 2

.globl ___tsan_cpu_arm64e_auth_jmp_sp
.private_extern ___tsan_cpu_arm64e_auth_jmp_sp
___tsan_cpu_arm64e_auth_jmp_sp:
  .cfi_startproc
  bti c

  mov x16, x0
  autdb x16, x1
  mov x17, x16
  xpacd x17
  cmp x16, x17
  b.eq 1f
  brk #0xc473
1:
  mov x0, x16
  ret

  .cfi_endproc
#endif  // defined(__APPLE__) && defined(__arm64e__)
