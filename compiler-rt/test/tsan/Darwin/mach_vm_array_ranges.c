// RUN: %clang_tsan %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
// UNSUPPORTED: iossim
//
// Verify that Mach VM fixed-range overflow does not wrap into an otherwise
// supported region and that a zero-size allocation never touches shadow data.

#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
  mach_vm_address_t addr = UINT64_MAX - 4095;
  kern_return_t result =
      mach_vm_allocate(mach_task_self(), &addr, 8192, VM_FLAGS_FIXED);
  if (result != KERN_NO_SPACE) {
    fprintf(stderr, "Expected KERN_NO_SPACE for overflowing Mach VM range\n");
    return 1;
  }

  addr = 0;
  // Zero-size requests are decided by Mach, not TSan's shadow-range code.
  (void)mach_vm_allocate(mach_task_self(), &addr, 0, VM_FLAGS_ANYWHERE);
  puts("Darwin array range validation: OK");
  return 0;
}

// CHECK: Darwin array range validation: OK
