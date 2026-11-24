// RUN: %clang_tsan -O1 %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='WARNING: ThreadSanitizer'
// UNSUPPORTED: iossim

#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
  const long native_page = sysconf(_SC_PAGESIZE);
  if (native_page <= 0)
    abort();
  const mach_vm_size_t page = (mach_vm_size_t)native_page;
  const mach_port_t task = mach_task_self();
  mach_vm_address_t base = 0;
  if (mach_vm_allocate(task, &base, 2 * page, VM_FLAGS_ANYWHERE) !=
      KERN_SUCCESS)
    abort();

  volatile unsigned char *bytes = (void *)base;
  bytes[page - 1] = 42;
  bytes[page] = 73;

  // XNU realigns the start before rounding size for a fixed allocation.
  // Overwrite only the first page; the following page must remain intact.
  mach_vm_address_t address = base + page - 1;
  if (mach_vm_allocate(task, &address, 1,
                       VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE) != KERN_SUCCESS ||
      address != base || bytes[page - 1] != 0 || bytes[page] != 73)
    abort();

  // Deallocation rounds the original interval, crossing the page boundary.
  if (mach_vm_deallocate(task, base + page - 1, 2) != KERN_SUCCESS)
    abort();
  puts("Mach allocation and deallocation geometry: OK");
  return 0;
}

// CHECK: Mach allocation and deallocation geometry: OK
