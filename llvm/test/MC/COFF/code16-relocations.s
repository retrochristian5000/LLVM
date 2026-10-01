# RUN: llvm-mc -filetype=obj -triple i386-pc-windows-code16 %s -o - | \
# RUN:   llvm-readobj --relocations - | FileCheck %s

        .text
        .code16
        .globl  _start
_start:
        .short  external
        callw   external

# CHECK: Relocations [
# CHECK: IMAGE_REL_I386_DIR16 external
# CHECK: IMAGE_REL_I386_REL16 external
