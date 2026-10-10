// Regression test for macOS arm64e ASan signal reporting. Pointer
// authentication on saved return addresses must not corrupt PC unwinding,
// and SP/FP must retain their original stack-address values.
//
// REQUIRES: arm64e-target-arch, asan-dynamic-runtime
// RUN: %clangxx_asan -O0 -g -fno-omit-frame-pointer %s -o %t
// RUN: not %run %t 2>&1 | FileCheck %s

#include <sys/mman.h>

__attribute__((noinline)) int fault_read(volatile char *p) { return *p; }

int main() {
  void *ptr = mmap(nullptr, 0x4000, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
  if (ptr == MAP_FAILED)
    return 2;
  return fault_read(static_cast<volatile char *>(ptr));
}

// CHECK: ERROR: AddressSanitizer: {{SEGV|BUS}} on unknown address
// CHECK: The signal is caused by a READ memory access.
// CHECK: #0 0x{{[0-9a-f]+}} in fault_read
// CHECK: #1 0x{{[0-9a-f]+}} in main
// CHECK: Register values:
