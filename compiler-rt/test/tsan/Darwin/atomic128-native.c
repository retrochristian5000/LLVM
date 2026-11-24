// REQUIRES: aarch64-target-arch
// RUN: %clang_tsan -O2 -pthread %s -o %t && %run %t 2>&1 | FileCheck %s --implicit-check-not='WARNING: ThreadSanitizer'

#include <assert.h>
#include <pthread.h>
#include <sanitizer/tsan_interface_atomic.h>
#include <stdio.h>

typedef __tsan_atomic128 Wide;
static _Alignas(16) Wide value;
static const int order = __tsan_memory_order_seq_cst;
static const Wide step = ((Wide)1 << 64) | 1;

static void *worker(void *unused) {
  (void)unused;
  for (int i = 0; i < 10000; ++i)
    __tsan_atomic128_fetch_add(&value, step, order);
  return 0;
}

int main(void) {
  __tsan_atomic128_store(&value, 0, order);
  pthread_t thread;
  assert(!pthread_create(&thread, 0, worker, 0));
  worker(0);
  assert(!pthread_join(thread, 0));
  assert(__tsan_atomic128_load(&value, order) == 20000 * step);
  Wide expected = 20000 * step;
  assert(__tsan_atomic128_compare_exchange_strong(
      &value, &expected, step, order, order));
  expected = 0;
  assert(!__tsan_atomic128_compare_exchange_strong(
      &value, &expected, 0, order, order));
  assert(expected == step);
  assert(__tsan_atomic128_fetch_nand(&value, step, order) == step);
  assert(__tsan_atomic128_load(&value, order) == ~step);
  puts("Darwin 128-bit atomics: OK");
  return 0;
}
// CHECK: Darwin 128-bit atomics: OK
