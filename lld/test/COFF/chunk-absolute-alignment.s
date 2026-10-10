# REQUIRES: x86
#
# A chunk with 8192-byte alignment must have its actual RVA aligned, even
# when the output section begins at the usual 4096-byte PE boundary.
# Aligning only the offset within the output section gives the wrong RVA.
#
# RUN: llvm-mc -filetype=obj -triple=x86_64-pc-windows %s -o %t.first.obj
# RUN: llvm-mc -filetype=obj -triple=x86_64-pc-windows -defsym SECOND=1 %s -o %t.aligned.obj
#
# RUN: lld-link /out:%t.single.exe /entry:aligned /subsystem:console /nodefaultlib %t.aligned.obj
# RUN: llvm-readobj --file-headers --sections %t.single.exe | FileCheck %s --check-prefix=SINGLE
# SINGLE: AddressOfEntryPoint: 0x2000
# SINGLE: Name: .text
# SINGLE: VirtualSize: 0x1001
#
# RUN: lld-link /out:%t.multiple.exe /entry:main /subsystem:console /nodefaultlib %t.first.obj %t.aligned.obj
# RUN: llvm-readobj --file-headers --sections %t.multiple.exe | FileCheck %s --check-prefix=MULTIPLE
# MULTIPLE: AddressOfEntryPoint: 0x1000
# MULTIPLE: Name: .text
# MULTIPLE: VirtualSize: 0x1001
#
        .text
.ifdef SECOND
        .p2align 13
        .globl aligned
aligned:
        retq
.else
        .globl main
main:
        retq
.endif
