//===-- tsan_array_aarch64.S -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Write TSan shadow cells for native AArch64. Each cell is four 32-bit
// RawShadow values: {value, empty, empty, empty}. The caller validates that
// [begin, end) is a nonempty, 16-byte-aligned shadow-memory range. The helper
// also accepts an empty range and never changes memory mapping state.
// x0: first shadow cell; x1: exclusive end; w2: first word per cell.

#if defined(__aarch64__)
.text
.p2align 2
#if defined(__APPLE__)
.globl ___tsan_shadow_set_aarch64
.private_extern ___tsan_shadow_set_aarch64
___tsan_shadow_set_aarch64:
#else
.globl __tsan_shadow_set_aarch64
.hidden __tsan_shadow_set_aarch64
.type __tsan_shadow_set_aarch64, %function
__tsan_shadow_set_aarch64:
#endif
  bti c
  cmp x0, x1
  b.hs 4f
  uxtw x2, w2
  sub x3, x1, x0
  // Four cells per iteration reduce loop-control work without assuming
  // a particular cache line size, enabling optional ISA extensions, or
  // writing beyond the requested range. A18 Pro can use this baseline
  // AArch64 sequence as well as earlier cores.
  cmp x3, #64
  b.lo 2f
1:
  stp x2, xzr, [x0]
  stp x2, xzr, [x0, #16]
  stp x2, xzr, [x0, #32]
  stp x2, xzr, [x0, #48]
  add x0, x0, #64
  sub x3, x3, #64
  cmp x3, #64
  b.hs 1b
2:
  cbz x3, 4f
3:
  stp x2, xzr, [x0], #16
  subs x3, x3, #16
  b.ne 3b
4:
  ret

#if !defined(__APPLE__)
.size __tsan_shadow_set_aarch64, .-__tsan_shadow_set_aarch64
.section .note.GNU-stack,"",%progbits
#endif
#endif  // __aarch64__
