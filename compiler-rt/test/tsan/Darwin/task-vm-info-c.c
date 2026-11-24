// RUN: %clang_tsan -O1 -std=c11 %s %S/../../../lib/tsan/mman/tsan_mman_vm_darwin.c -o %t
// RUN: %run %t

#include "../../../lib/tsan/mman/tsan_mman_vm_darwin.h"
#include <mach/mach.h>
#include <mach/task_info.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
  if (__tsan_darwin_task_vm_info(NULL) != TSAN_DARWIN_TASK_VM_UNAVAILABLE)
    abort();

  TsanDarwinTaskVmInfo vm;
  const int status = __tsan_darwin_task_vm_info(&vm);
  if (status == TSAN_DARWIN_TASK_VM_UNAVAILABLE || !vm.virtual_bytes ||
      vm.resident_bytes > vm.virtual_bytes)
    abort();
  const long page = sysconf(_SC_PAGESIZE);
  if (page <= 0 || vm.page_size != (size_t)page)
    abort();

#ifdef TASK_VM_INFO_REV2_COUNT
  task_vm_info_data_t direct = {0};
  mach_msg_type_number_t count = TASK_VM_INFO_REV2_COUNT;
  if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&direct,
                &count) != KERN_SUCCESS)
    abort();
  // Compare stable bounds, not live resident sizes or region counts.
  if (count >= TASK_VM_INFO_REV2_COUNT &&
      direct.min_address < direct.max_address) {
    if (status != TSAN_DARWIN_TASK_VM_BOUNDS ||
        vm.min_address != direct.min_address ||
        vm.max_address != direct.max_address)
      abort();
  } else if (status != TSAN_DARWIN_TASK_VM_BASIC ||
             vm.min_address || vm.max_address) {
    abort();
  }
#else
  if (status != TSAN_DARWIN_TASK_VM_BASIC || vm.min_address || vm.max_address)
    abort();
#endif
  return 0;
}
