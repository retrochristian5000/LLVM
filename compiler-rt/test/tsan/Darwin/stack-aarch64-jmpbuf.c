// Verify TSan records and restores 16-byte-aligned AArch64 call-site SPs
// through each libc setjmp/longjmp variant, including nested stack frames.
//
// REQUIRES: aarch64-target-arch
// RUN: %clang_tsan -O1 %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'

#include <setjmp.h>
#include <stdio.h>

static __attribute__((noinline)) void jump_plain(jmp_buf *p) {
  longjmp(*p, 7);
}

static __attribute__((noinline)) void jump_underscore(jmp_buf *p) {
  _longjmp(*p, 11);
}

static __attribute__((noinline)) void jump_signal(sigjmp_buf *p) {
  siglongjmp(*p, 13);
}

static __attribute__((noinline)) int check_stack(unsigned depth) {
  volatile unsigned char marker[32];
  marker[0] = (unsigned char)depth;
  if (depth && !check_stack(depth - 1))
    return 0;

  jmp_buf first;
  int a = setjmp(first);
  if (!a)
    jump_plain(&first);

  jmp_buf second;
  int b = _setjmp(second);
  if (!b)
    jump_underscore(&second);

  sigjmp_buf third;
  int c = sigsetjmp(third, 0);
  if (!c)
    jump_signal(&third);

  return a == 7 && b == 11 && c == 13 && marker[0] == depth;
}

int main(void) {
  for (unsigned i = 0; i < 32; ++i) {
    if (!check_stack(i % 5))
      return 1;
  }
  puts("AArch64 setjmp stack ABI: OK");
  return 0;
}

// CHECK: AArch64 setjmp stack ABI: OK
