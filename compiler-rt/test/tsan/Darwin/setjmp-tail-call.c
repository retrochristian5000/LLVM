// Ensure TSan's Darwin setjmp interceptors can tail-call libc and preserve
// the original return context. On arm64e the libc branch uses a PAC-aware
// linker stub; a plain branch through a GOT function pointer is not valid.
//
// RUN: %clang_tsan -O1 %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'

#include <setjmp.h>
#include <stdio.h>

static __attribute__((noinline)) int check_setjmp(void) {
  jmp_buf env;
  int value = setjmp(env);
  if (value == 0)
    longjmp(env, 17);
  return value;
}

static __attribute__((noinline)) int check__setjmp(void) {
  jmp_buf env;
  int value = _setjmp(env);
  if (value == 0)
    _longjmp(env, 19);
  return value;
}

static __attribute__((noinline)) int check_sigsetjmp(void) {
  sigjmp_buf env;
  int value = sigsetjmp(env, 1);
  if (value == 0)
    siglongjmp(env, 23);
  return value;
}

int main(void) {
  if (check_setjmp() != 17 || check__setjmp() != 19 ||
      check_sigsetjmp() != 23)
    return 1;
  puts("setjmp interceptors: OK");
  return 0;
}

// CHECK: setjmp interceptors: OK
