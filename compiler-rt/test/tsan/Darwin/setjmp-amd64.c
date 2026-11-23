// RUN: %clang_tsan -O1 %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
// REQUIRES: x86_64-target-arch

// Darwin AMD64 interceptors preserve the System V argument registers and
// tail-call libc while retaining the original setjmp stack context.

#include <setjmp.h>
#include <stdio.h>

static __attribute__((noinline)) int TestSetJmp(int target) {
  jmp_buf env;
  volatile int observed = setjmp(env);
  if (observed == 0)
    longjmp(env, target);
  return observed;
}

static __attribute__((noinline)) int TestUnderscoreSetJmp(int target) {
  jmp_buf env;
  volatile int observed = _setjmp(env);
  if (observed == 0)
    _longjmp(env, target);
  return observed;
}

static __attribute__((noinline)) int TestSigSetJmp(int target) {
  sigjmp_buf env;
  volatile int observed = sigsetjmp(env, 1);
  if (observed == 0)
    siglongjmp(env, target);
  return observed;
}

int main(void) {
  for (int i = 1; i < 1000; ++i) {
    if (TestSetJmp(i) != i ||
        TestUnderscoreSetJmp(i + 1) != i + 1 ||
        TestSigSetJmp(i + 2) != i + 2)
      return 1;
  }
  puts("Darwin AMD64 setjmp interceptors: OK");
  return 0;
}

// CHECK: Darwin AMD64 setjmp interceptors: OK
