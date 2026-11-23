//===-- tsan_ignore_aarch64.S ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Checked AArch64 ignore-depth transitions for thread-local 32-bit counters.
// Accept only +1 (enter) or -1 (leave). A null pointer, invalid delta,
// negative counter, underflow, or signed overflow returns 0 without a write.
// ThreadState offsets and FastState bitfield layouts are deliberately avoided.

#if defined(__aarch64__)

#include "sanitizer_common/sanitizer_asm.h"
#include "builtins/assembly.h"

TEXT_SECTION
#if defined(__APPLE__)
.align 3
#endif

.globl ASM_SYMBOL(__tsan_ignore_aarch64_depth_step)
#if defined(__APPLE__)
.private_extern ASM_SYMBOL(__tsan_ignore_aarch64_depth_step)
#else
ASM_HIDDEN(ASM_SYMBOL(__tsan_ignore_aarch64_depth_step))
#endif
ASM_TYPE_FUNCTION(ASM_SYMBOL(__tsan_ignore_aarch64_depth_step))
ASM_SYMBOL(__tsan_ignore_aarch64_depth_step):
  CFI_STARTPROC
  BTI_C

  // x0: int *depth, w1: delta. Return w0=1 on success, 0 on failure.
  // Preserve x19-x29, stack pointer, link register, and all SIMD registers.
  cbz     x0, .Ltsan_ignore_invalid
  cmp     w1, #1
  b.eq    .Ltsan_ignore_enter
  cmn     w1, #1
  b.ne    .Ltsan_ignore_invalid

  ldr     w2, [x0]
  cbz     w2, .Ltsan_ignore_invalid
  tbnz    w2, #31, .Ltsan_ignore_invalid
  sub     w2, w2, #1
  str     w2, [x0]
  mov     w0, #1
  ret

.Ltsan_ignore_enter:
  ldr     w2, [x0]
  tbnz    w2, #31, .Ltsan_ignore_invalid
  adds    w3, w2, #1
  b.vs    .Ltsan_ignore_invalid
  str     w3, [x0]
  mov     w0, #1
  ret

.Ltsan_ignore_invalid:
  mov     w0, #0
  ret

  CFI_ENDPROC
ASM_SIZE(ASM_SYMBOL(__tsan_ignore_aarch64_depth_step))

NO_EXEC_STACK_DIRECTIVE
GNU_PROPERTY_BTI_PAC_GCS

#endif  // defined(__aarch64__)
