# REQUIRES: aarch64
#
## Authenticated relocations only belong in versioned arm64e Mach-O objects.
## Keep ordinary arm64 separate: its object header must not advertise an ABI
## that cannot consume ARM64_RELOC_AUTHENTICATED_POINTER.
#
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macosx13.0 %s -o %t.o
# RUN: llvm-objdump --macho --private-header %t.o | FileCheck %s --check-prefix=ARM64E-HDR
# RUN: llvm-readobj -r --expand-relocs %t.o | FileCheck %s --check-prefix=ARM64E-RELOC
# RUN: not llvm-mc -filetype=obj -triple=arm64-apple-macosx13.0 %s -o %t-arm64.o 2>&1 | FileCheck %s --check-prefix=ARM64-REJECT
#
# ARM64E-HDR: MH_MAGIC_64
# ARM64E-HDR-SAME: ARM64
# ARM64E-HDR-SAME: E PAC00
# ARM64E-HDR-SAME: OBJECT
# ARM64E-RELOC: Type: ARM64_RELOC_UNSIGNED (0)
# ARM64E-RELOC-COUNT-4: Type: ARM64_RELOC_AUTHENTICATED_POINTER (11)
# ARM64-REJECT: error: authenticated pointer relocation requires arm64e Mach-O

.text
.globl _whp_target
_whp_target:
  ret

.data
.p2align 3
.quad _whp_target@AUTH(ia,42,addr)
.quad _whp_target@AUTH(ib,513)
.quad _whp_target@AUTH(da,7,addr)
.quad _whp_target@AUTH(db,65535)
.quad _whp_target
