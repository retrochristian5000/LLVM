# REQUIRES: aarch64
# RUN: split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=arm64-apple-macosx11.0 %t/ref.s -o %t/ref.o
# RUN: llvm-mc -filetype=obj -triple=arm64-apple-macosx11.0 %t/valid.s -o %t/valid.o
# RUN: llvm-mc -filetype=obj -triple=arm64-apple-macosx11.0 %t/overflow.s -o %t/overflow.o
# RUN: %no-lsystem-lld -arch arm64 -dylib -no_fixup_chains %t/ref.o %t/valid.o -o %t/valid.dylib
# RUN: llvm-objdump --full-contents --section=__data %t/valid.dylib | FileCheck %s --check-prefix=DATA
# RUN: not %no-lsystem-lld -arch arm64 -dylib -no_fixup_chains %t/ref.o %t/overflow.o -o %t/overflow.dylib 2>&1 | FileCheck %s --check-prefix=RANGE

# DATA: Contents of section __DATA,__data:
# DATA-NEXT: {{[0-9a-f]+}} 00000080 ffffffff
# RANGE: relocation UNSIGNED is out of range:
# RANGE-SAME: 4294967296 is not in [0, 4294967295]

#--- ref.s
.data
.globl _ptr
.p2align 2
_ptr:
  .long _lo
  .long _hi

#--- valid.s
.globl _lo, _hi
_lo = 0x80000000
_hi = 0xffffffff

#--- overflow.s
.globl _lo, _hi
_lo = 0x100000000
_hi = 0xffffffff
