// RUN: %clangxx_tsan -O1 %s -o %t -pthread -framework Foundation
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'

// Darwin pthread TLS and GCD worker lifecycle handling must keep thread
// startup, completion, joining, and worker teardown synchronized.

#include <dispatch/dispatch.h>
#include <pthread.h>
#include <stdio.h>

struct Shared {
  int pthread_value;
  int dispatch_value;
};

static void *PthreadWork(void *arg) {
  static_cast<Shared *>(arg)->pthread_value = 17;
  return nullptr;
}

static void DispatchWork(void *arg) {
  static_cast<Shared *>(arg)->dispatch_value = 23;
}

int main() {
  Shared shared = {};
  pthread_t thread;
  if (pthread_create(&thread, nullptr, PthreadWork, &shared) != 0)
    return 1;
  if (pthread_join(thread, nullptr) != 0)
    return 2;

  dispatch_group_t group = dispatch_group_create();
  dispatch_group_async_f(group, dispatch_get_global_queue(0, 0), &shared,
                         DispatchWork);
  if (dispatch_group_wait(group, DISPATCH_TIME_FOREVER) != 0)
    return 3;

  if (shared.pthread_value != 17 || shared.dispatch_value != 23)
    return 4;
  puts("Darwin TSan pthread and GCD lifecycle: OK");
  return 0;
}

// CHECK: Darwin TSan pthread and GCD lifecycle: OK
