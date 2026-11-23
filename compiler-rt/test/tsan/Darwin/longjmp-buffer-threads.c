// Exercise Darwin's libc jump-buffer decoding on multiple pthreads.
// The saved stack pointer is used to match each non-local jump against
// TSan's recorded setjmp frame (including arm64e pointer authentication).
//
// RUN: %clang_tsan -O1 %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
// RUN: %clang_tsan -O2 %s -o %t.opt
// RUN: %run %t.opt 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'

#include <pthread.h>
#include <setjmp.h>
#include <stdio.h>

enum { kThreads = 4, kRounds = 256 };

static __attribute__((noinline)) void jump_plain(jmp_buf *env) {
  longjmp(*env, 17);
}

static __attribute__((noinline)) void jump_underscore(jmp_buf *env) {
  _longjmp(*env, 19);
}

static __attribute__((noinline)) void jump_signal(sigjmp_buf *env) {
  siglongjmp(*env, 23);
}

static __attribute__((noinline)) int test_plain(void) {
  jmp_buf env;
  int result = setjmp(env);
  if (result == 0)
    jump_plain(&env);
  return result == 17;
}

static __attribute__((noinline)) int test_underscore(void) {
  jmp_buf env;
  int result = _setjmp(env);
  if (result == 0)
    jump_underscore(&env);
  return result == 19;
}

static __attribute__((noinline)) int test_signal(void) {
  sigjmp_buf env;
  // The zero flag must not require a saved OS signal mask.
  int result = sigsetjmp(env, 0);
  if (result == 0)
    jump_signal(&env);
  return result == 23;
}

static void *worker(void *unused) {
  (void)unused;
  for (int i = 0; i < kRounds; ++i) {
    if (!test_plain() || !test_underscore() || !test_signal())
      return (void *)1;
  }
  return NULL;
}

int main(void) {
  pthread_t threads[kThreads];
  for (int i = 0; i < kThreads; ++i) {
    if (pthread_create(&threads[i], NULL, worker, NULL) != 0)
      return 1;
  }
  for (int i = 0; i < kThreads; ++i) {
    void *status = NULL;
    if (pthread_join(threads[i], &status) != 0 || status != NULL)
      return 1;
  }
  puts("Darwin longjmp buffer matching: OK");
  return 0;
}

// CHECK: Darwin longjmp buffer matching: OK
