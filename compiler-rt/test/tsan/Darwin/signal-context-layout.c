// RUN: %clang_tsan -std=c11 -O1 %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
//
// Check Darwin's public signal ABI against the opaque copies in
// tsan_interceptors_posix.cpp. In particular, the machine context is not
// contiguous with the ucontext header on an alternate signal stack.

#include <stddef.h>
#include <signal.h>
#include <sys/ucontext.h>
#include <stdio.h>

#if defined(__LP64__)
_Static_assert(sizeof(siginfo_t) == 104,
               "update Darwin TSan siginfo copy size for this SDK");
_Static_assert(offsetof(ucontext_t, uc_mcontext) +
                   sizeof(((ucontext_t *)0)->uc_mcontext) == 56,
               "update Darwin TSan ucontext header size for this SDK");
#endif

int main(void) {
  puts("Darwin signal ABI layout: OK");
  return 0;
}

// CHECK: Darwin signal ABI layout: OK
