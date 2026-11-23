// AArch64 setjmp/sigsetjmp interceptors (ELF and Darwin).
// Keep register and stack-frame handling ISA-specific; the signal-mask
// semantics belong to the target C library. On arm64e, leave libc tail
// branches to the Mach-O linker so it can generate authenticated stubs.
#if defined(__aarch64__)

#include "sanitizer_common/sanitizer_asm.h"
#include "builtins/assembly.h"
#include "../reg/tsan_reg_aarch64.h"

TEXT_SECTION
#if defined(__APPLE__)
.align 3
#endif

ASM_HIDDEN(__tsan_setjmp)
#if !defined(__APPLE__)
.comm _ZN14__interception11real_setjmpE,8,8
#endif
.globl ASM_SYMBOL_INTERCEPTOR(setjmp)
ASM_TYPE_FUNCTION(ASM_SYMBOL_INTERCEPTOR(setjmp))
ASM_SYMBOL_INTERCEPTOR(setjmp):
  CFI_STARTPROC
  BTI_C

  // Save frame/link register
  stp     x29, x30, [sp, -32]!
  CFI_DEF_CFA_OFFSET (32)
  CFI_OFFSET (TSAN_AARCH64_DWARF_FP, -32)
  CFI_OFFSET (TSAN_AARCH64_DWARF_LR, -24)

  // Adjust the SP for previous frame
  add     x29, sp, 0
  CFI_DEF_CFA_REGISTER (TSAN_AARCH64_DWARF_FP)

  // Save env parameter
  str     x0, [sp, 16]
  CFI_OFFSET (TSAN_AARCH64_DWARF_X0, -16)

  // Obtain SP, first argument to `void __tsan_setjmp(uptr sp)`
  add     x0, x29, 32

  // call tsan interceptor
  bl      ASM_SYMBOL(__tsan_setjmp)

  // Restore env parameter
  ldr     x0, [sp, 16]
  CFI_RESTORE (TSAN_AARCH64_DWARF_X0)

  // Restore frame/link register
  ldp     x29, x30, [sp], 32
  CFI_RESTORE (TSAN_AARCH64_DWARF_FP)
  CFI_RESTORE (TSAN_AARCH64_DWARF_LR)
  CFI_DEF_CFA (TSAN_AARCH64_DWARF_SP, 0)

  // tail jump to libc setjmp
#if !defined(__APPLE__)
  adrp    x16, :got:_ZN14__interception11real_setjmpE
  ldr     x16, [x16, #:got_lo12:_ZN14__interception11real_setjmpE]
  ldr     x16, [x16]
  br      x16
#else
  // Let the Mach-O linker generate an authenticated arm64e stub.
  b       ASM_SYMBOL(setjmp)
#endif

  CFI_ENDPROC
ASM_SIZE(ASM_SYMBOL_INTERCEPTOR(setjmp))

#if !defined(__APPLE__)
.comm _ZN14__interception12real__setjmpE,8,8
#endif
.globl ASM_SYMBOL_INTERCEPTOR(_setjmp)
ASM_TYPE_FUNCTION(ASM_SYMBOL_INTERCEPTOR(_setjmp))
ASM_SYMBOL_INTERCEPTOR(_setjmp):
  CFI_STARTPROC
  BTI_C

  // Save frame/link register
  stp     x29, x30, [sp, -32]!
  CFI_DEF_CFA_OFFSET (32)
  CFI_OFFSET (TSAN_AARCH64_DWARF_FP, -32)
  CFI_OFFSET (TSAN_AARCH64_DWARF_LR, -24)

  // Adjust the SP for previous frame
  add     x29, sp, 0
  CFI_DEF_CFA_REGISTER (TSAN_AARCH64_DWARF_FP)

  // Save env parameter
  str     x0, [sp, 16]
  CFI_OFFSET (TSAN_AARCH64_DWARF_X0, -16)

  // Obtain SP, first argument to `void __tsan_setjmp(uptr sp)`
  add     x0, x29, 32

  // call tsan interceptor
  bl      ASM_SYMBOL(__tsan_setjmp)

  // Restore env parameter
  ldr     x0, [sp, 16]
  CFI_RESTORE (TSAN_AARCH64_DWARF_X0)

  // Restore frame/link register
  ldp     x29, x30, [sp], 32
  CFI_RESTORE (TSAN_AARCH64_DWARF_FP)
  CFI_RESTORE (TSAN_AARCH64_DWARF_LR)
  CFI_DEF_CFA (TSAN_AARCH64_DWARF_SP, 0)

  // tail jump to libc setjmp
#if !defined(__APPLE__)
  adrp    x16, :got:_ZN14__interception12real__setjmpE
  ldr     x16, [x16, #:got_lo12:_ZN14__interception12real__setjmpE]
  ldr     x16, [x16]
  br      x16
#else
  b       ASM_SYMBOL(_setjmp)
#endif

  CFI_ENDPROC
ASM_SIZE(ASM_SYMBOL_INTERCEPTOR(_setjmp))

#if !defined(__APPLE__)
.comm _ZN14__interception14real_sigsetjmpE,8,8
#endif
.globl ASM_SYMBOL_INTERCEPTOR(sigsetjmp)
ASM_TYPE_FUNCTION(ASM_SYMBOL_INTERCEPTOR(sigsetjmp))
ASM_SYMBOL_INTERCEPTOR(sigsetjmp):
  CFI_STARTPROC
  BTI_C

  // Save frame/link register
  stp     x29, x30, [sp, -32]!
  CFI_DEF_CFA_OFFSET (32)
  CFI_OFFSET (TSAN_AARCH64_DWARF_FP, -32)
  CFI_OFFSET (TSAN_AARCH64_DWARF_LR, -24)

  // Adjust the SP for previous frame
  add     x29, sp, 0
  CFI_DEF_CFA_REGISTER (TSAN_AARCH64_DWARF_FP)

  // Save env and savesigs parameter
  stp     x0, x1, [sp, 16]
  CFI_OFFSET (TSAN_AARCH64_DWARF_X0, -16)
  CFI_OFFSET (TSAN_AARCH64_DWARF_X1, -8)

  // Obtain SP, first argument to `void __tsan_setjmp(uptr sp)`
  add     x0, x29, 32

  // call tsan interceptor
  bl      ASM_SYMBOL(__tsan_setjmp)

  // Restore env and savesigs parameter
  ldp     x0, x1, [sp, 16]
  CFI_RESTORE (TSAN_AARCH64_DWARF_X0)
  CFI_RESTORE (TSAN_AARCH64_DWARF_X1)

  // Restore frame/link register
  ldp     x29, x30, [sp], 32
  CFI_RESTORE (TSAN_AARCH64_DWARF_FP)
  CFI_RESTORE (TSAN_AARCH64_DWARF_LR)
  CFI_DEF_CFA (TSAN_AARCH64_DWARF_SP, 0)

  // tail jump to libc sigsetjmp
#if !defined(__APPLE__)
  adrp    x16, :got:_ZN14__interception14real_sigsetjmpE
  ldr     x16, [x16, #:got_lo12:_ZN14__interception14real_sigsetjmpE]
  ldr     x16, [x16]
  br      x16
#else
  b       ASM_SYMBOL(sigsetjmp)
#endif
  CFI_ENDPROC
ASM_SIZE(ASM_SYMBOL_INTERCEPTOR(sigsetjmp))

#if !defined(__APPLE__)
.comm _ZN14__interception16real___sigsetjmpE,8,8
.globl ASM_SYMBOL_INTERCEPTOR(__sigsetjmp)
ASM_TYPE_FUNCTION(ASM_SYMBOL_INTERCEPTOR(__sigsetjmp))
ASM_SYMBOL_INTERCEPTOR(__sigsetjmp):
  CFI_STARTPROC
  BTI_C

  // Save frame/link register
  stp     x29, x30, [sp, -32]!
  CFI_DEF_CFA_OFFSET (32)
  CFI_OFFSET (TSAN_AARCH64_DWARF_FP, -32)
  CFI_OFFSET (TSAN_AARCH64_DWARF_LR, -24)

  // Adjust the SP for previous frame
  add     x29, sp, 0
  CFI_DEF_CFA_REGISTER (TSAN_AARCH64_DWARF_FP)

  // Save env and savesigs parameter
  stp     x0, x1, [sp, 16]
  CFI_OFFSET (TSAN_AARCH64_DWARF_X0, -16)
  CFI_OFFSET (TSAN_AARCH64_DWARF_X1, -8)

  // Obtain SP, first argument to `void __tsan_setjmp(uptr sp)`
  add     x0, x29, 32

  // call tsan interceptor
  bl      ASM_SYMBOL(__tsan_setjmp)

  // Restore env and savesigs parameter
  ldp     x0, x1, [sp, 16]
  CFI_RESTORE (TSAN_AARCH64_DWARF_X0)
  CFI_RESTORE (TSAN_AARCH64_DWARF_X1)

  // Restore frame/link register
  ldp     x29, x30, [sp], 32
  CFI_RESTORE (TSAN_AARCH64_DWARF_FP)
  CFI_RESTORE (TSAN_AARCH64_DWARF_LR)
  CFI_DEF_CFA (TSAN_AARCH64_DWARF_SP, 0)

  // tail jump to libc __sigsetjmp
#if !defined(__APPLE__)
  adrp    x16, :got:_ZN14__interception16real___sigsetjmpE
  ldr     x16, [x16, #:got_lo12:_ZN14__interception16real___sigsetjmpE]
  ldr     x16, [x16]
#endif
  br      x16
  CFI_ENDPROC
ASM_SIZE(ASM_SYMBOL_INTERCEPTOR(__sigsetjmp))
#endif

NO_EXEC_STACK_DIRECTIVE

GNU_PROPERTY_BTI_PAC_GCS

#endif
