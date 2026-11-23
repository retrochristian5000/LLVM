//===-- tsan_buff_aarch64.S -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Reverse a buffer of 64-bit program-counter words in place. This function
// moves raw words and does not authenticate or modify saved PC values.
// x0: non-null buffer, x1: element count. Clobbers caller-saved x0-x5 only.

#if defined(__aarch64__)
.text
.p2align 2
#if defined(__APPLE__)
.globl ___tsan_reverse_pc_buffer_aarch64
.private_extern ___tsan_reverse_pc_buffer_aarch64
___tsan_reverse_pc_buffer_aarch64:
#else
.globl __tsan_reverse_pc_buffer_aarch64
.hidden __tsan_reverse_pc_buffer_aarch64
.type __tsan_reverse_pc_buffer_aarch64, %function
__tsan_reverse_pc_buffer_aarch64:
#endif
  bti c
  // Zero or one entry requires no memory access.
  lsr x2, x1, #1
  cbz x2, 2f
  sub x1, x1, #1
  add x3, x0, x1, lsl #3
1:
  ldr x4, [x0]
  ldr x5, [x3]
  str x5, [x0]
  str x4, [x3]
  add x0, x0, #8
  sub x3, x3, #8
  subs x2, x2, #1
  b.ne 1b
2:
  ret

#if !defined(__APPLE__)
.size __tsan_reverse_pc_buffer_aarch64, .-__tsan_reverse_pc_buffer_aarch64
.section .note.GNU-stack,"",%progbits
#endif
#endif  // __aarch64__
