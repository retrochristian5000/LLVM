// RUN: %clang_tsan -O2 %s -o %t && %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
// REQUIRES: aarch64-target-arch

#include <assert.h>
#include <sanitizer/tsan_interface_atomic.h>
#include <stdio.h>

#define CHECK_NAND(bits, type)                                                \
  do {                                                                       \
    __tsan_atomic##bits value = (type)0x5a;                                    \
    type old = __tsan_atomic##bits##_fetch_nand(                               \
        &value, (type)0x33, __tsan_memory_order_seq_cst);                       \
    assert(old == (type)0x5a);                                                \
    assert((type)__tsan_atomic##bits##_load(                                 \
               &value, __tsan_memory_order_seq_cst) ==                        \
           (type)~((type)0x5a & (type)0x33));                                  \
  } while (0)

int main(void) {
  CHECK_NAND(8, unsigned char);
  CHECK_NAND(16, unsigned short);
  CHECK_NAND(32, unsigned int);
  CHECK_NAND(64, unsigned long long);
  puts("Darwin native atomic NAND: OK");
  return 0;
}

// CHECK: Darwin native atomic NAND: OK
