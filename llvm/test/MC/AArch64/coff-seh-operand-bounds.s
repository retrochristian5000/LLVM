// RUN: llvm-mc -triple aarch64-windows -filetype=obj --defsym GOOD=1 %s -o %t-arm64.obj
// RUN: llvm-readobj --file-headers %t-arm64.obj | FileCheck %s --check-prefix=ARM64
// RUN: llvm-mc -triple aarch64-pc-windows-msvc -filetype=obj --defsym GOOD=1 %s -o %t-msvc.obj
// RUN: llvm-readobj --file-headers %t-msvc.obj | FileCheck %s --check-prefix=ARM64
// RUN: llvm-mc -triple aarch64-w64-windows-gnu -filetype=obj --defsym GOOD=1 %s -o %t-gnu.obj
// RUN: llvm-readobj --file-headers %t-gnu.obj | FileCheck %s --check-prefix=ARM64
// RUN: llvm-mc -triple arm64ec-pc-windows-msvc -filetype=obj --defsym GOOD=1 %s -o %t-ec.obj
// RUN: llvm-readobj --file-headers %t-ec.obj | FileCheck %s --check-prefix=ARM64EC
//
// RUN: not llvm-mc -triple aarch64-windows -filetype=obj --defsym BAD=1 %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=BAD
// RUN: not llvm-mc -triple aarch64-pc-windows-msvc -filetype=obj --defsym BAD=1 %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=BAD
// RUN: not llvm-mc -triple aarch64-w64-windows-gnu -filetype=obj --defsym BAD=1 %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=BAD
// RUN: not llvm-mc -triple arm64ec-pc-windows-msvc -filetype=obj --defsym BAD=1 %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=BAD
//
// ARM64: Format: COFF-ARM64
// ARM64EC: Format: COFF-ARM64EC

.ifdef GOOD
.text
.globl legal_stack
legal_stack:
  .seh_proc legal_stack
  sub sp, sp, #16
  .seh_stackalloc 16
  .seh_endprologue
  add sp, sp, #16
  ret
  .seh_endproc

.globl legal_fp
legal_fp:
  .seh_proc legal_fp
  add x29, sp, #2040
  .seh_add_fp 2040
  .seh_endprologue
  ret
  .seh_endproc
.endif

.ifdef BAD
.text
bad:
  .seh_proc bad
  nop
  .seh_stackalloc -16
  // BAD: error: .seh_stackalloc size must be a positive multiple of 16
  nop
  .seh_stackalloc 0
  // BAD: error: .seh_stackalloc size must be a positive multiple of 16
  nop
  .seh_stackalloc 8
  // BAD: error: .seh_stackalloc size must be a positive multiple of 16
  nop
  .seh_stackalloc 0x10000000
  // BAD: error: .seh_stackalloc size must be a positive multiple of 16
  nop
  .seh_stackalloc 0x100000000
  // BAD: error: .seh_stackalloc size must be a positive multiple of 16
  nop
  .seh_add_fp -8
  // BAD: error: .seh_add_fp offset must be a multiple of 8
  nop
  .seh_add_fp 3
  // BAD: error: .seh_add_fp offset must be a multiple of 8
  nop
  .seh_add_fp 2048
  // BAD: error: .seh_add_fp offset must be a multiple of 8
  nop
  .seh_add_fp 0x100000000
  // BAD: error: .seh_add_fp offset must be a multiple of 8
  .seh_endprologue
  ret
  .seh_endproc
.endif
