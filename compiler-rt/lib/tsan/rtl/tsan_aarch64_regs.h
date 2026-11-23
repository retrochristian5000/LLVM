//===-- tsan_aarch64_regs.h -------------------------------------*- C -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// AArch64 DWARF register numbers (Arm AADWARF64), shared by ELF and Mach-O
// assembly unwind directives. These are NOT instruction register aliases:
// A64 operands already use x0-x30, sp, and v0-v31 directly.
//
// Keep ARM64e pointer-authentication unwind handling separate from the
// register numbering. Do not reinterpret SP (DWARF 31) as an X31 register.

#ifndef TSAN_AARCH64_REGS_H
#define TSAN_AARCH64_REGS_H

#if defined(__aarch64__)

// General-purpose registers X0-X30, DWARF numbers 0-30.
#define TSAN_AARCH64_DWARF_X0 0
#define TSAN_AARCH64_DWARF_X1 1
#define TSAN_AARCH64_DWARF_X2 2
#define TSAN_AARCH64_DWARF_X3 3
#define TSAN_AARCH64_DWARF_X4 4
#define TSAN_AARCH64_DWARF_X5 5
#define TSAN_AARCH64_DWARF_X6 6
#define TSAN_AARCH64_DWARF_X7 7
#define TSAN_AARCH64_DWARF_X8 8
#define TSAN_AARCH64_DWARF_X9 9
#define TSAN_AARCH64_DWARF_X10 10
#define TSAN_AARCH64_DWARF_X11 11
#define TSAN_AARCH64_DWARF_X12 12
#define TSAN_AARCH64_DWARF_X13 13
#define TSAN_AARCH64_DWARF_X14 14
#define TSAN_AARCH64_DWARF_X15 15
#define TSAN_AARCH64_DWARF_X16 16
#define TSAN_AARCH64_DWARF_X17 17
#define TSAN_AARCH64_DWARF_X18 18
#define TSAN_AARCH64_DWARF_X19 19
#define TSAN_AARCH64_DWARF_X20 20
#define TSAN_AARCH64_DWARF_X21 21
#define TSAN_AARCH64_DWARF_X22 22
#define TSAN_AARCH64_DWARF_X23 23
#define TSAN_AARCH64_DWARF_X24 24
#define TSAN_AARCH64_DWARF_X25 25
#define TSAN_AARCH64_DWARF_X26 26
#define TSAN_AARCH64_DWARF_X27 27
#define TSAN_AARCH64_DWARF_X28 28
#define TSAN_AARCH64_DWARF_X29 29
#define TSAN_AARCH64_DWARF_X30 30

// The stack pointer and program counter have distinct DWARF numbers.
#define TSAN_AARCH64_DWARF_SP 31
#define TSAN_AARCH64_DWARF_PC 32
#define TSAN_AARCH64_DWARF_FP TSAN_AARCH64_DWARF_X29
#define TSAN_AARCH64_DWARF_LR TSAN_AARCH64_DWARF_X30

// Advanced SIMD/floating-point V0-V31, DWARF numbers 64-95.
#define TSAN_AARCH64_DWARF_V0 64
#define TSAN_AARCH64_DWARF_V1 65
#define TSAN_AARCH64_DWARF_V2 66
#define TSAN_AARCH64_DWARF_V3 67
#define TSAN_AARCH64_DWARF_V4 68
#define TSAN_AARCH64_DWARF_V5 69
#define TSAN_AARCH64_DWARF_V6 70
#define TSAN_AARCH64_DWARF_V7 71
#define TSAN_AARCH64_DWARF_V8 72
#define TSAN_AARCH64_DWARF_V9 73
#define TSAN_AARCH64_DWARF_V10 74
#define TSAN_AARCH64_DWARF_V11 75
#define TSAN_AARCH64_DWARF_V12 76
#define TSAN_AARCH64_DWARF_V13 77
#define TSAN_AARCH64_DWARF_V14 78
#define TSAN_AARCH64_DWARF_V15 79
#define TSAN_AARCH64_DWARF_V16 80
#define TSAN_AARCH64_DWARF_V17 81
#define TSAN_AARCH64_DWARF_V18 82
#define TSAN_AARCH64_DWARF_V19 83
#define TSAN_AARCH64_DWARF_V20 84
#define TSAN_AARCH64_DWARF_V21 85
#define TSAN_AARCH64_DWARF_V22 86
#define TSAN_AARCH64_DWARF_V23 87
#define TSAN_AARCH64_DWARF_V24 88
#define TSAN_AARCH64_DWARF_V25 89
#define TSAN_AARCH64_DWARF_V26 90
#define TSAN_AARCH64_DWARF_V27 91
#define TSAN_AARCH64_DWARF_V28 92
#define TSAN_AARCH64_DWARF_V29 93
#define TSAN_AARCH64_DWARF_V30 94
#define TSAN_AARCH64_DWARF_V31 95

#endif  // defined(__aarch64__)

#endif  // TSAN_AARCH64_REGS_H
