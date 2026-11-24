// RUN: %clangxx_tsan -O1 -pthread -fobjc-arc %s -o %t -framework Foundation
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'

#import <Foundation/Foundation.h>
#include <objc/objc-sync.h>
#include <pthread.h>
#include <stdio.h>

extern "C" BOOL objc_sync_try_enter(id obj) __attribute__((weak_import));

struct Shared {
  __unsafe_unretained id lock;
  int value;
  int failed;
  int released;
};

static void *Worker(void *arg) {
  Shared *s = static_cast<Shared *>(arg);
  if (objc_sync_try_enter(s->lock)) {
    objc_sync_exit(s->lock);
    __atomic_store_n(&s->failed, -1, __ATOMIC_RELAXED);
    return (void *)1;
  }
  __atomic_store_n(&s->failed, 1, __ATOMIC_RELAXED);
  while (!__atomic_load_n(&s->released, __ATOMIC_RELAXED)) {}
  while (!objc_sync_try_enter(s->lock)) {}
  ++s->value;
  return objc_sync_exit(s->lock) == OBJC_SYNC_SUCCESS ? nullptr : (void *)1;
}

static bool Exercise(id lock) {
  Shared s = {lock, 0, 0, 0};
  if (objc_sync_enter(lock) != OBJC_SYNC_SUCCESS)
    return false;
  if (!objc_sync_try_enter(lock) || objc_sync_exit(lock) != OBJC_SYNC_SUCCESS)
    return false;
  pthread_t thread;
  if (pthread_create(&thread, nullptr, Worker, &s))
    return false;
  while (!__atomic_load_n(&s.failed, __ATOMIC_RELAXED)) {}
  ++s.value;
  if (objc_sync_exit(lock) != OBJC_SYNC_SUCCESS)
    return false;
  __atomic_store_n(&s.released, 1, __ATOMIC_RELAXED);
  void *result = nullptr;
  return !pthread_join(thread, &result) && !result && s.failed == 1 &&
         s.value == 2;
}

int main() {
  if (!objc_sync_try_enter) {
    puts("Objective-C try-lock unavailable");
    return 0;
  }
  @autoreleasepool {
    if (!Exercise([NSObject new]) || !Exercise(@42))
      return 1;
  }
  puts("Objective-C try-lock synchronization: OK");
  return 0;
}

// CHECK: Objective-C try-lock {{synchronization: OK|unavailable}}
