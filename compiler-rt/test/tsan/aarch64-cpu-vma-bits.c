// RUN: %clang -O1 -Wall -Wextra -Werror -I%S/../../lib %s %S/../../lib/tsan/rtl/tsan_cpu_aarch64.S -o %t
// RUN: %run %t 2>&1 | FileCheck %s
// REQUIRES: aarch64-target-arch

// Test the ISA helper independently of TSan's platform shadow mapping.

#include <stdint.h>
#include <stdio.h>

extern uint64_t __tsan_cpu_aarch64_vma_bits(uint64_t value);

int main(void) {
  static const struct {
    uint64_t value;
    uint64_t bits;
  } cases[] = {
      {1ULL, 1},
      {2ULL, 2},
      {(1ULL << 38) - 1, 38},
      {1ULL << 38, 39},
      {1ULL << 41, 42},
      {1ULL << 46, 47},
      {1ULL << 47, 48},
      {1ULL << 63, 64},
      {UINT64_MAX, 64},
  };

  for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    uint64_t actual = __tsan_cpu_aarch64_vma_bits(cases[i].value);
    if (actual != cases[i].bits) {
      fprintf(stderr, "VMA width mismatch for case %u: %llu != %llu\n", i,
              (unsigned long long)actual, (unsigned long long)cases[i].bits);
      return 1;
    }
  }
  puts("TSan AArch64 CPU VMA bits: OK");
  return 0;
}

// CHECK: TSan AArch64 CPU VMA bits: OK
