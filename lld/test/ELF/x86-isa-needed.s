# REQUIRES: x86
# RUN: llvm-mc -filetype=obj -triple=x86_64 -defsym=WORD=8 -defsym=ISA=3 -defsym=CET=1 %s -o %t.a
# RUN: llvm-mc -filetype=obj -triple=x86_64 -defsym=WORD=8 -defsym=ISA=5 -defsym=CET=1 %s -o %t.b
# RUN: llvm-mc -filetype=obj -triple=x86_64 -defsym=WORD=8 -defsym=ISA=0 -defsym=CET=0 %s -o %t.c
# RUN: ld.lld -shared %t.a %t.b -o %t.so
# RUN: llvm-readelf -n %t.so | FileCheck %s --check-prefixes=WITH-CET,ISA
# RUN: ld.lld -shared %t.a %t.b %t.c -o %t.so
# RUN: llvm-readelf -n %t.so | FileCheck %s --check-prefixes=NO-CET,ISA
# RUN: ld.lld -r %t.a %t.b -o %t.r
# RUN: llvm-readelf -n %t.r | FileCheck %s --check-prefixes=WITH-CET,ISA
# RUN: llvm-mc -filetype=obj -triple=i386 -defsym=WORD=4 -defsym=ISA=3 -defsym=CET=1 %s -o %t.32a
# RUN: llvm-mc -filetype=obj -triple=i386 -defsym=WORD=4 -defsym=ISA=5 -defsym=CET=1 %s -o %t.32b
# RUN: ld.lld -shared %t.32a %t.32b -o %t.32.so
# RUN: llvm-readelf -n %t.32.so | FileCheck %s --check-prefixes=WITH-CET,ISA

# WITH-CET: x86 feature: IBT
# NO-CET-NOT: x86 feature:
# ISA: x86 ISA needed: x86-64-baseline, x86-64-v2, x86-64-v3{{$}}

.text
nop

.if ISA || CET
.section .note.gnu.property,"a",@note
.balign WORD
.long 4
.long .Lend - .Ldesc
.long 5
.asciz "GNU"
.Ldesc:
.if CET
.long 0xc0000002
.long 4
.long CET
.balign WORD
.endif
.if ISA
.long 0xc0008002
.long 4
.long ISA
.balign WORD
.endif
.Lend:
.endif
