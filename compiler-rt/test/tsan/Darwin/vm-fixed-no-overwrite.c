// RUN: %clang_tsan %s -o %t && %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
// Verify the Darwin fixed reservation contract used by TSan startup:
// a fixed anonymous map must not replace an existing mapping without
// VM_FLAGS_OVERWRITE.

#include <mach/mach.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
  const size_t page = (size_t)getpagesize();
  unsigned char *p = mmap(NULL, page, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANON, -1, 0);
  if (p == MAP_FAILED)
    return 1;
  p[0] = 0x5a;
  vm_address_t addr = (vm_address_t)p;
  kern_return_t kr = vm_map(mach_task_self(), &addr, page, 0,
                            VM_FLAGS_FIXED, MACH_PORT_NULL, 0, FALSE,
                            VM_PROT_READ | VM_PROT_WRITE,
                            VM_PROT_READ | VM_PROT_WRITE, VM_INHERIT_COPY);
  if (kr == KERN_SUCCESS || p[0] != 0x5a)
    return 2;
  if (munmap(p, page))
    return 3;
  puts("Darwin fixed VM reservation preserves existing mapping");
  return 0;
}

// CHECK: Darwin fixed VM reservation preserves existing mapping
