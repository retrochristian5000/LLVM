// RUN: %clang_tsan -O1 %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'

// Darwin libplatform restores the saved signal mask and resets the kernel's
// alternate-stack status when siglongjmp follows sigsetjmp(env, 1). TSan's
// interception must preserve both behaviors on arm64/arm64e and x86_64.

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static sigjmp_buf checkpoint;
static volatile sig_atomic_t handled;

static void on_signal(int sig) {
  if (sig == SIGUSR1) {
    handled = 1;
    siglongjmp(checkpoint, 23);
  }
}

int main(void) {
  size_t stack_size = 4 * SIGSTKSZ;
  if (stack_size < 128 * 1024)
    stack_size = 128 * 1024;
  void *memory = malloc(stack_size);
  if (!memory)
    return 1;

  stack_t stack = {0};
  stack.ss_sp = memory;
  stack.ss_size = stack_size;
  if (sigaltstack(&stack, NULL) != 0)
    return 1;

  struct sigaction action = {0};
  action.sa_handler = on_signal;
  action.sa_flags = SA_ONSTACK;
  sigemptyset(&action.sa_mask);
  if (sigaction(SIGUSR1, &action, NULL) != 0)
    return 1;

  int result = sigsetjmp(checkpoint, 1);
  if (result == 0) {
    if (raise(SIGUSR1) != 0)
      return 1;
    return 1;
  }
  if (result != 23 || !handled)
    return 1;

  stack_t current;
  if (sigaltstack(NULL, &current) != 0)
    return 1;
  if (current.ss_flags & SS_ONSTACK)
    return 1;

  stack_t disabled = {0};
  disabled.ss_flags = SS_DISABLE;
  if (sigaltstack(&disabled, NULL) != 0)
    return 1;
  free(memory);

  puts("Darwin siglongjmp alternate stack: OK");
  return 0;
}

// CHECK: Darwin siglongjmp alternate stack: OK
