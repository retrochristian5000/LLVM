// RUN: %clang_tsan -O1 -pthread %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='FATAL: ThreadSanitizer'
// REQUIRES: linux

// CPU/ABI VMA-width validation must complete before TSan starts threads and
// installs its architecture-dependent shadow-memory mapping.

#include <pthread.h>
#include <stdio.h>

static void *worker(void *arg) {
  int *value = (int *)arg;
  *value = 17;
  return NULL;
}

int main(void) {
  pthread_t thread;
  int value = 0;
  if (pthread_create(&thread, NULL, worker, &value) != 0)
    return 1;
  if (pthread_join(thread, NULL) != 0 || value != 17)
    return 2;
  puts("TSan CPU VMA startup: OK");
  return 0;
}

// CHECK: TSan CPU VMA startup: OK
