// RUN: %clang -O1 -Wall -Wextra -Werror -I%S/../../lib %s -o %t
// RUN: %run %t | FileCheck %s
// REQUIRES: aarch64-target-arch

// Check the AADWARF64 CFI numbering, independently of the system ABI's
// register context layout. This is shared by Linux AArch64 and Darwin arm64e.
#include "tsan/rtl/tsan_aarch64_regs.h"
#include <stdio.h>

_Static_assert(TSAN_AARCH64_DWARF_X0 == 0, "X0 register number");
_Static_assert(TSAN_AARCH64_DWARF_X1 == 1, "X1 register number");
_Static_assert(TSAN_AARCH64_DWARF_X8 == 8, "X8 register number");
_Static_assert(TSAN_AARCH64_DWARF_FP == 29, "X29 frame pointer number");
_Static_assert(TSAN_AARCH64_DWARF_LR == 30, "X30 link register number");
_Static_assert(TSAN_AARCH64_DWARF_SP == 31, "SP number (not X31)");
_Static_assert(TSAN_AARCH64_DWARF_PC == 32, "PC number");
_Static_assert(TSAN_AARCH64_DWARF_V0 == 64, "V0 SIMD register number");
_Static_assert(TSAN_AARCH64_DWARF_V8 == 72, "V8 SIMD register number");
_Static_assert(TSAN_AARCH64_DWARF_V31 == 95, "V31 SIMD register number");
_Static_assert(TSAN_AARCH64_DWARF_X30 == TSAN_AARCH64_DWARF_LR,
               "LR aliases X30");
_Static_assert(TSAN_AARCH64_DWARF_X29 == TSAN_AARCH64_DWARF_FP,
               "FP aliases X29");

int main(void) {
  puts("TSan AArch64 DWARF register numbering: OK");
  return 0;
}

// CHECK: TSan AArch64 DWARF register numbering: OK
