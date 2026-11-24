// RUN: %clang_tsan -O1 -pthread %s -o %t
// RUN: %env_tsan_opts=ignore_noninstrumented_modules=0 %run %t padding 2>&1 | FileCheck %s --check-prefix=CLEAN --implicit-check-not='WARNING: ThreadSanitizer'
// RUN: %env_tsan_opts=ignore_noninstrumented_modules=0 not %run %t pixels 2>&1 | FileCheck %s --check-prefix=RACE
//
// Model a synchronous CPU-side Metal upload with two BGRA pixels per row
// and eight bytes of padding. No GPU is required for this regression.
#include <pthread.h>
#include <sanitizer/tsan_interface.h>
#include <stdio.h>
#include <string.h>

static unsigned char image[32];
static int ready;

static void *writer(void *arg) {
  const int pixels = *(const int *)arg;
  image[pixels ? 16 : 8] = 42;
  __atomic_store_n(&ready, 1, __ATOMIC_RELAXED);
  return 0;
}

int main(int argc, char **argv) {
  int pixels = argc == 2 && !strcmp(argv[1], "pixels");
  pthread_t thread;
  if (pthread_create(&thread, 0, writer, &pixels))
    return 1;
  while (!__atomic_load_n(&ready, __ATOMIC_RELAXED)) {}
  // The relaxed flag schedules the test but does not establish happens-before.
  __tsan_read_range(image, 8);
  __tsan_read_range(image + 16, 8);
  pthread_join(thread, 0);
  puts("CPU upload range checked");
  return 0;
}
// CLEAN: CPU upload range checked
// RACE: WARNING: ThreadSanitizer: data race
