// REQUIRES: aarch64-target-arch
// RUN: %clang_tsan -O1 -pthread %s -o %t
// RUN: %env_tsan_opts=ignore_noninstrumented_modules=0 not %run %t 2>&1 | FileCheck %s

#include <pthread.h>
#include <sanitizer/tsan_interface_atomic.h>

static _Alignas(16) __tsan_atomic128 value;
static int ready;

static void *worker(void *unused) {
  (void)unused;
  // A non-atomic access to the second shadow cell, not the first.
  ((volatile unsigned char *)&value)[12] = 42;
  __atomic_store_n(&ready, 1, __ATOMIC_RELAXED);
  return 0;
}

int main(void) {
  pthread_t thread;
  if (pthread_create(&thread, 0, worker, 0))
    return 1;
  while (!__atomic_load_n(&ready, __ATOMIC_RELAXED)) {}
  __tsan_atomic128_store(&value, 0, __tsan_memory_order_relaxed);
  pthread_join(thread, 0);
  return 0;
}
// CHECK: WARNING: ThreadSanitizer: data race
