// Linux libcs may store a mangled stack pointer in jmp_buf. TSan must
// recover the saved SP before unwinding its shadow stack. In particular,
// glibc 2.44 no longer uses the old x86_64 pointer-guard location.
//
// REQUIRES: linux
// RUN: %clang_tsan -O1 %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'

#include <setjmp.h>
#include <stdio.h>

static __attribute__((noinline)) void leave_frame(jmp_buf *env, int value) {
  longjmp(*env, value);
}

static __attribute__((noinline)) int plain_jump(void) {
  jmp_buf env;
  int result = setjmp(env);
  if (result == 0)
    leave_frame(&env, 17);
  return result;
}

static __attribute__((noinline)) void leave_signal_frame(sigjmp_buf *env) {
  siglongjmp(*env, 23);
}

static __attribute__((noinline)) int signal_jump(void) {
  sigjmp_buf env;
  int result = sigsetjmp(env, 0);
  if (result == 0)
    leave_signal_frame(&env);
  return result;
}

int main(void) {
  // Run from different call frames repeatedly, checking the recovered SP
  // and that previous jump-buffer entries are cleaned up correctly.
  for (int i = 0; i < 10000; ++i) {
    if (plain_jump() != 17 || signal_jump() != 23)
      return 1;
  }

  puts("longjmp buffer mangling: OK");
  return 0;
}

// CHECK: longjmp buffer mangling: OK
