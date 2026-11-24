// REQUIRES: tsan-objc-runtime
// RUN: %clang_tsan -std=c11 -fno-objc-arc %s -lobjc -o %t
// RUN: %env_tsan_opts=verbosity=2 %run %t 2>&1 | FileCheck %s
// CHECK: ThreadSanitizer: native Objective-C ABI: BOOL=[[ENC:[Bc]]] size=1
// CHECK: ThreadSanitizer: Objective-C image placement: supported
// CHECK-NOT: WARNING: ThreadSanitizer
// CHECK: application Objective-C ABI: BOOL=[[ENC]] size=1
// CHECK-NOT: WARNING: ThreadSanitizer

#include <objc/runtime.h>
#include <stdio.h>

int main(void) {
  fprintf(stderr, "application Objective-C ABI: BOOL=%s size=%zu\n",
          @encode(BOOL), sizeof(BOOL));
  return 0;
}
