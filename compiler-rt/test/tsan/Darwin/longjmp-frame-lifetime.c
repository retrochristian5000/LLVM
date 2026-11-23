// Verify two active setjmp environments at the same native stack depth.
// RUN: %clang_tsan -O1 %s -o %t && %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
// RUN: %clang_tsan -O2 %s -o %t.opt && %run %t.opt 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
#include <setjmp.h>
#include <stdio.h>
__attribute__((noinline)) static void jump_to(jmp_buf *env, int value) {
  longjmp(*env, value);
}
__attribute__((noinline)) static int two_environments(void) {
  jmp_buf outer;
  jmp_buf inner;
  volatile int first = setjmp(outer);
  if (first == 0) {
    volatile int second = setjmp(inner);
    if (second == 0)
      jump_to(&inner, 11);
    if (second != 11)
      return 0;
    jump_to(&outer, 17);
  }
  return first == 17;
}
int main(void) {
  for (int i = 0; i < 1024; ++i) {
    if (!two_environments())
      return 1;
  }
  puts("Darwin setjmp frame lifetime: OK");
  return 0;
}
// CHECK: Darwin setjmp frame lifetime: OK
