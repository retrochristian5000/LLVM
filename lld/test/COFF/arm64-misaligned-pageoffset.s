# REQUIRES: aarch64

# RUN: llvm-mc -filetype=obj -triple=aarch64-windows %s -o %t.obj
# RUN: not lld-link /entry:main /subsystem:console /out:%t.exe %t.obj 2>&1 | FileCheck %s

# Keep PAGEOFFSET_12L strict, but make failures actionable.  The target is
# deliberately one byte into .rdata while the instruction is an eight-byte
# load, so the low page offset cannot be represented by the scaled immediate.
# CHECK: error: misaligned ldr/str offset: 0x1@f9400000 with align 2^3 from bad@

    .text
    .globl main
main:
    adrp x0, bad
    ldr  x0, [x0, :lo12:bad]
    ret

    .section .rdata, "dr"
    .byte 0
    .globl bad
bad:
    .byte 0, 0, 0, 0, 0, 0, 0, 0
