// RUN: %clang_tsan -O1 -pthread %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
//
// An asynchronous signal can be dispatched after the original signal stack
// frame has returned. The deferred Darwin ucontext must own its machine state.

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/ucontext.h>

static pthread_t target;
static int sent;
static volatile sig_atomic_t saw_machine_context;

static void on_signal(int signo, siginfo_t *info, void *context) {
  if (signo != SIGUSR1 || !info || !context)
    return;
  const ucontext_t *uc = (const ucontext_t *)context;
  if (!uc->uc_mcontext || uc->uc_mcsize < sizeof(*uc->uc_mcontext))
    return;
#if defined(__aarch64__)
# if __DARWIN_OPAQUE_ARM_THREAD_STATE64
  if (uc->uc_mcontext->__ss.__opaque_pc)
# else
  if (uc->uc_mcontext->__ss.__pc)
# endif
    saw_machine_context = 1;
#elif defined(__x86_64__)
  if (uc->uc_mcontext->__ss.__rip)
    saw_machine_context = 1;
#else
# error "Darwin TSan machine-context regression requires a supported CPU"
#endif
}

static void *send_signal(void *unused) {
  (void)unused;
  int rc = pthread_kill(target, SIGUSR1);
  __atomic_store_n(&sent, rc ? -1 : 1, __ATOMIC_RELEASE);
  return NULL;
}

int main(void) {
  size_t size = 4 * SIGSTKSZ;
  if (size < 128 * 1024)
    size = 128 * 1024;
  void *memory = malloc(size);
  if (!memory)
    return 1;
  stack_t stack = {0};
  stack.ss_sp = memory;
  stack.ss_size = size;
  if (sigaltstack(&stack, NULL))
    return 1;

  struct sigaction action = {0};
  action.sa_sigaction = on_signal;
  action.sa_flags = SA_ONSTACK | SA_SIGINFO;
  sigemptyset(&action.sa_mask);
  if (sigaction(SIGUSR1, &action, NULL))
    return 1;

  target = pthread_self();
  pthread_t sender;
  if (pthread_create(&sender, NULL, send_signal, NULL))
    return 1;

  // Do not enter an interceptor until the sender has requested delivery.
  while (!__atomic_load_n(&sent, __ATOMIC_ACQUIRE)) {
    __asm__ __volatile__("" ::: "memory");
  }
  struct timeval now;
  for (int i = 0; i < 100000 && !saw_machine_context; ++i)
    gettimeofday(&now, NULL);
  if (pthread_join(sender, NULL))
    return 1;

  stack_t disabled = {0};
  disabled.ss_flags = SS_DISABLE;
  if (sigaltstack(&disabled, NULL))
    return 1;
  free(memory);

  if (__atomic_load_n(&sent, __ATOMIC_ACQUIRE) != 1 ||
      !saw_machine_context)
    return 1;
  puts("Darwin deferred signal machine context: OK");
  return 0;
}

// CHECK: Darwin deferred signal machine context: OK
