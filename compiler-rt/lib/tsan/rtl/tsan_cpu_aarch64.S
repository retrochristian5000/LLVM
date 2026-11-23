//===-- tsan_cpu_aarch64.S -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// AArch64 CPU primitives for TSan, independent of OS and libc interceptors.
// The caller supplies its frame address; an out-of-line call must not sample
// its own stack frame when selecting the runtime shadow-memory mapping.

#if defined(__aarch64__)

#include "sanitizer_common/sanitizer_asm.h"
#include "builtins/assembly.h"

TEXT_SECTION
#if defined(__APPLE__)
.align 3
#endif

.globl ASM_SYMBOL(__tsan_cpu_aarch64_vma_bits)
ASM_HIDDEN(ASM_SYMBOL(__tsan_cpu_aarch64_vma_bits))
ASM_TYPE_FUNCTION(ASM_SYMBOL(__tsan_cpu_aarch64_vma_bits))
ASM_SYMBOL(__tsan_cpu_aarch64_vma_bits):
  CFI_STARTPROC
  BTI_C

  // x0: nonzero frame address. Return floor(log2(x0)) + 1 in x0.
  // x1 is caller-saved under AAPCS64. No privileged register access.
  clz     x1, x0
  mov     x0, #64
  sub     x0, x0, x1
  ret

  CFI_ENDPROC
ASM_SIZE(ASM_SYMBOL(__tsan_cpu_aarch64_vma_bits))

NO_EXEC_STACK_DIRECTIVE
GNU_PROPERTY_BTI_PAC_GCS

#endif  // defined(__aarch64__)
