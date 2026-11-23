//===-- tsan_stack_aarch64.S ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// AArch64 stack-pointer validation for TSan's setjmp/longjmp bookkeeping.
// At an AAPCS64 public interface, SP must be 16-byte aligned. Validate the
// captured/decoded SP without modifying it, the stack, or signed return PCs.
//
// Input: x0 = captured SP; return: w0 = 1 if nonzero and aligned, else 0.
// No stack accesses, calls, or callee-saved register modifications.
//
//===----------------------------------------------------------------------===//

#if defined(__aarch64__)
.text
.p2align 2

#if defined(__APPLE__)
.globl ___tsan_stack_aarch64_valid_sp
.private_extern ___tsan_stack_aarch64_valid_sp
___tsan_stack_aarch64_valid_sp:
#else
.globl __tsan_stack_aarch64_valid_sp
.hidden __tsan_stack_aarch64_valid_sp
.type __tsan_stack_aarch64_valid_sp, %function
__tsan_stack_aarch64_valid_sp:
#endif
  bti c
  cbz x0, 1f
  tst x0, #15
  cset w0, eq
  ret
1:
  mov w0, wzr
  ret

#if !defined(__APPLE__)
.size __tsan_stack_aarch64_valid_sp, .-__tsan_stack_aarch64_valid_sp
.section .note.GNU-stack,"",%progbits
#endif

#endif  // defined(__aarch64__)
