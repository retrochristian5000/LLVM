// RUN: %clang_tsan -O1 %s -o %t
// RUN: %env_tsan_opts=ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t mach adjacent
// RUN: %env_tsan_opts=ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t legacy adjacent
// RUN: %env_tsan_opts=ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t mach failed
// RUN: %env_tsan_opts=ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t legacy failed
// RUN: not %env_tsan_opts=ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t mach source 2>&1 | FileCheck %s
// RUN: not %env_tsan_opts=ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t mach destination 2>&1 | FileCheck %s
// RUN: not %env_tsan_opts=ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t legacy source 2>&1 | FileCheck %s
// RUN: not %env_tsan_opts=ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t legacy destination 2>&1 | FileCheck %s
// UNSUPPORTED: ios, iossim
// CHECK: WARNING: ThreadSanitizer: data race

#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static mach_vm_address_t base;
static mach_vm_size_t page;
static int legacy;
static enum { Adjacent, Failed, Source, Destination } mode;

static void *copy_pages(void *unused) {
  (void)unused;
  mach_vm_address_t src = mode == Failed ? 0 : base;
  mach_vm_address_t dst = base + 2 * page;
  kern_return_t kr = legacy
      ? vm_copy(mach_task_self(), (vm_address_t)src, (vm_size_t)page,
                (vm_address_t)dst)
      : mach_vm_copy(mach_task_self(), src, page, dst);
  if ((kr == KERN_SUCCESS) == (mode == Failed))
    abort();
  return NULL;
}

static void *compete(void *unused) {
  (void)unused;
  volatile unsigned char *bytes = (void *)(uintptr_t)base;
  if (mode == Source)
    bytes[page - 1] = 7;
  else if (mode == Destination || mode == Failed)
    bytes[3 * page - 1] = 9;
  else {
    bytes[page] = 7;
    bytes[3 * page] = 9;
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc != 3)
    abort();
  legacy = strcmp(argv[1], "legacy") == 0;
  if (!legacy && strcmp(argv[1], "mach") != 0)
    abort();
  if (!strcmp(argv[2], "adjacent"))
    mode = Adjacent;
  else if (!strcmp(argv[2], "failed"))
    mode = Failed;
  else if (!strcmp(argv[2], "source"))
    mode = Source;
  else if (!strcmp(argv[2], "destination"))
    mode = Destination;
  else
    abort();

  const long native_page = sysconf(_SC_PAGESIZE);
  if (native_page <= 0)
    abort();
  page = (mach_vm_size_t)native_page;
  if (mach_vm_allocate(mach_task_self(), &base, 4 * page, VM_FLAGS_ANYWHERE) !=
      KERN_SUCCESS)
    abort();
  volatile unsigned char *bytes = (void *)(uintptr_t)base;
  bytes[0] = 42;
  pthread_t copier, writer;
  if (pthread_create(&copier, NULL, copy_pages, NULL) ||
      pthread_create(&writer, NULL, compete, NULL))
    abort();
  if (pthread_join(copier, NULL) || pthread_join(writer, NULL))
    abort();
  if (mode != Failed && bytes[2 * page] != 42)
    abort();
  if (mach_vm_deallocate(mach_task_self(), base, 4 * page) != KERN_SUCCESS)
    abort();
  return 0;
}
