# REQUIRES: x86
#
# /aligncomm's argument is a log2 alignment in bytes. Common symbols can
# request stronger alignment than COFF section headers can encode (8192).
# The linker must align the symbol's actual RVA, not just its section offset.
#
# RUN: llvm-mc -filetype=obj -triple=x86_64-pc-windows %s -o %t.obj
# RUN: llvm-mc -filetype=obj -triple=x86_64-pc-windows -defsym DIRECTIVE=1 %s -o %t.directive.obj
#
# RUN: lld-link /out:%t.16k.exe /entry:main /subsystem:console /nodefaultlib /aligncomm:big,14 %t.obj
# RUN: llvm-readobj --sections %t.16k.exe | FileCheck %s --check-prefix=ALIGN16K
# ALIGN16K: Name: .data
# ALIGN16K-NEXT: VirtualSize: 0x2004
#
# Repeating an option preserves the strongest requirement, regardless of order.
# RUN: lld-link /out:%t.repeated.exe /entry:main /subsystem:console /nodefaultlib /aligncomm:big,14 /aligncomm:big,4 %t.obj
# RUN: llvm-readobj --sections %t.repeated.exe | FileCheck %s --check-prefix=ALIGN16K
#
# /aligncomm in the COFF .drectve section must follow the same rules.
# RUN: lld-link /out:%t.directive.exe /entry:main /subsystem:console /nodefaultlib %t.directive.obj
# RUN: llvm-readobj --sections %t.directive.exe | FileCheck %s --check-prefix=ALIGN16K
#
# Larger BSS alignments do not require a correspondingly large output file.
# RUN: lld-link /out:%t.64k.exe /entry:main /subsystem:console /nodefaultlib /aligncomm:big,16 %t.obj
# RUN: llvm-readobj --sections %t.64k.exe | FileCheck %s --check-prefix=ALIGN64K
# ALIGN64K: Name: .data
# ALIGN64K-NEXT: VirtualSize: 0xE004
#
# Reject shifts that would be undefined or unrepresentable in uint32_t.
# RUN: not lld-link /out:%t.neg.exe /entry:main /subsystem:console /nodefaultlib /aligncomm:big,-1 %t.obj 2>&1 | FileCheck %s --check-prefix=NEGATIVE
# NEGATIVE: error: /aligncomm: invalid argument: big,-1
# RUN: not lld-link /out:%t.toobig.exe /entry:main /subsystem:console /nodefaultlib /aligncomm:big,32 %t.obj 2>&1 | FileCheck %s --check-prefix=TOOBIG
# TOOBIG: error: /aligncomm: invalid argument: big,32
# RUN: not lld-link /out:%t.nonnumeric.exe /entry:main /subsystem:console /nodefaultlib /aligncomm:big,nope %t.obj 2>&1 | FileCheck %s --check-prefix=NONNUMERIC
# NONNUMERIC: error: /aligncomm: invalid argument: big,nope
#
        .text
        .globl main
main:
        movabsq $big, %rax
        retq

        .comm big, 4

.ifdef DIRECTIVE
        .section .drectve,"yn"
        .ascii " /aligncomm:big,14"
.endif
