// RUN: %clangxx_tsan -O1 -pthread -fobjc-arc %s -o %t -framework Foundation
// RUN: %run %t 2>&1 | FileCheck %s

// Exercise @synchronized through objc_sync_enter/exit with a regular object
// and a potential tagged pointer (NSNumber representation depends on OS/CPU).

#import <Foundation/Foundation.h>
#include <pthread.h>
#include <stdio.h>

struct Shared {
  __unsafe_unretained id lock;
  int count;
};

static void *Worker(void *argument) {
  Shared *shared = static_cast<Shared *>(argument);
  @autoreleasepool {
    for (int i = 0; i < 2000; ++i) {
      @synchronized(shared->lock) {
        ++shared->count;
      }
    }
  }
  return nullptr;
}

static bool Exercise(id lock) {
  Shared shared = {lock, 0};
  pthread_t first;
  pthread_t second;
  if (pthread_create(&first, nullptr, Worker, &shared) != 0)
    return false;
  if (pthread_create(&second, nullptr, Worker, &shared) != 0) {
    pthread_join(first, nullptr);
    return false;
  }
  pthread_join(first, nullptr);
  pthread_join(second, nullptr);
  return shared.count == 4000;
}

int main() {
  @autoreleasepool {
    NSObject *ordinary = [NSObject new];
    NSNumber *candidate = @(42);
    if (!Exercise(ordinary) || !Exercise(candidate))
      return 1;
  }
  puts("Darwin Objective-C synchronization: OK");
  return 0;
}

// CHECK: Darwin Objective-C synchronization: OK
// CHECK-NOT: WARNING: ThreadSanitizer
