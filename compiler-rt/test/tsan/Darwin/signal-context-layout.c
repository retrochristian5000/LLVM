// RUN: %clang_tsan -std=c11 -O1 %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
//
// Check Darwin's public signal ABI against the native-size copies in
// tsan_interceptors_darwin.cpp. The machine context is separate from the
// ucontext header on an alternate signal stack.

#include <stddef.h>
#include <signal.h>
#include <sys/ucontext.h>
#include <stdio.h>

#if defined(__LP64__)
_Static_assert(sizeof(siginfo_t) <= 128,
               "Darwin TSan signal buffer cannot hold this siginfo_t");
_Static_assert(sizeof(ucontext_t) == 56,
               "update Darwin TSan context buffer for this SDK");
#endif

int main(void) {
  puts("Darwin signal ABI layout: OK");
  return 0;
}

// CHECK: Darwin signal ABI layout: OK
