// RUN: %clang_tsan -O1 -pthread %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
//
// Defer an asynchronous signal while running on an alternate signal stack.
// Darwin may supply a null context pointer, and its ucontext header is much
// smaller than Linux's. TSan must neither overread the stack nor turn a null
// context into a non-null pointer when replaying the user handler.

#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

static pthread_t main_thread;
static int sent;
static volatile sig_atomic_t handled;

static void handler(int sig, siginfo_t *info, void *ctx) {
  (void)ctx;
  if (sig == SIGUSR1 && info)
    handled = 1;
}

static void *send_to_main(void *unused) {
  (void)unused;
  int rc = pthread_kill(main_thread, SIGUSR1);
  __atomic_store_n(&sent, rc ? -1 : 1, __ATOMIC_RELEASE);
  return 0;
}

int main(void) {
  // Keep the alternate stack large enough for sanitizer instrumentation.
  size_t stack_size = 4 * SIGSTKSZ;
  if (stack_size < 128 * 1024)
    stack_size = 128 * 1024;
  void *stack_memory = malloc(stack_size);
  if (!stack_memory)
    return 1;
  stack_t stack = {0};
  stack.ss_sp = stack_memory;
  stack.ss_size = stack_size;
  if (sigaltstack(&stack, 0) != 0)
    return 1;

  struct sigaction action = {0};
  action.sa_sigaction = handler;
  action.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sigemptyset(&action.sa_mask);
  if (sigaction(SIGUSR1, &action, 0) != 0)
    return 1;

  main_thread = pthread_self();
  pthread_t thread;
  if (pthread_create(&thread, 0, send_to_main, 0) != 0)
    return 1;

  // gettimeofday is intercepted by TSan and processes pending signals.
  struct timeval now;
  while (__atomic_load_n(&sent, __ATOMIC_ACQUIRE) == 0)
    gettimeofday(&now, 0);
  for (int i = 0; i < 100000 && !handled; ++i)
    gettimeofday(&now, 0);
  if (pthread_join(thread, 0) != 0)
    return 1;

  stack_t disabled = {0};
  disabled.ss_flags = SS_DISABLE;
  if (sigaltstack(&disabled, 0) != 0)
    return 1;
  free(stack_memory);

  if (__atomic_load_n(&sent, __ATOMIC_ACQUIRE) != 1 || !handled)
    return 1;
  puts("asynchronous signal context: OK");
  return 0;
}

// CHECK: asynchronous signal context: OK
