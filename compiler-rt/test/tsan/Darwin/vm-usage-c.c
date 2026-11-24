// RUN: %clang_tsan -O1 -std=c11 %s %S/../../../lib/tsan/mman/tsan_mman_vm_darwin.c -o %t
// RUN: %run %t

#include "../../../lib/tsan/mman/tsan_mman_vm_darwin.h"
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
  const long native_page = sysconf(_SC_PAGESIZE);
  if (native_page <= 0)
    abort();
  const size_t page = (size_t)native_page;
  TsanDarwinVmUsage usage = {1, 1};
  if (__tsan_darwin_vm_usage(2, 1, page, &usage) ||
      usage.resident_bytes || usage.dirty_bytes)
    abort();
  if (!__tsan_darwin_vm_usage(1, 1, page, &usage) ||
      usage.resident_bytes || usage.dirty_bytes)
    abort();
  if (__tsan_darwin_vm_usage(1, 2, 3, &usage))
    abort();

  // Isolate the middle entry with inaccessible guard pages. The VM query
  // must not dereference those guards or infer data from adjacent entries.
  mach_vm_address_t base = 0;
  const mach_port_t task = mach_task_self();
  if (mach_vm_allocate(task, &base, 3 * page, VM_FLAGS_ANYWHERE) != KERN_SUCCESS)
    abort();
  if (mach_vm_protect(task, base, page, FALSE, VM_PROT_NONE) != KERN_SUCCESS ||
      mach_vm_protect(task, base + 2 * page, page, FALSE, VM_PROT_NONE) !=
          KERN_SUCCESS)
    abort();

  volatile unsigned char *payload = (void *)(uintptr_t)(base + page);
  *payload = 42;
  // Residency is sampled and may change under memory pressure; do not
  // demand that one particular resident/dirty count remain constant.
  if (!__tsan_darwin_vm_usage(base + page, base + 2 * page, page, &usage) ||
      usage.resident_bytes % page || usage.dirty_bytes % page)
    abort();
  if (__tsan_darwin_vm_usage(base + page + 1, base + 2 * page, page, &usage) ||
      usage.resident_bytes || usage.dirty_bytes)
    abort();
  if (mach_vm_deallocate(task, base, 3 * page) != KERN_SUCCESS)
    abort();
  return 0;
}
