// Regression for _setjmp/_longjmp on Darwin: the latter must unwind TSan's
// shadow call stack. Missing the interceptor leaks one function frame per
// iteration, eventually overflowing the shadow stack.
//
// RUN: %clang_tsan -O1 %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'

#include <setjmp.h>
#include <stdio.h>

static __attribute__((noinline)) void jump_out(jmp_buf *env) {
  _longjmp(*env, 37);
}

int main(void) {
  jmp_buf env;

  // Long enough to expose the shadow-stack growth seen without interception.
  for (int i = 0; i < 10000000; ++i) {
    int result = _setjmp(env);
    if (result == 0)
      jump_out(&env);
    if (result != 37)
      return 1;
  }

  puts("_longjmp shadow stack: OK");
  return 0;
}

// CHECK: _longjmp shadow stack: OK
