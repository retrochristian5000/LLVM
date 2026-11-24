// RUN: %clang_tsan -O1 -pthread %s -o %t && %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
// RUN: %clang_tsan -O2 -pthread %s -o %t.opt && %run %t.opt 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
//
// A deferred signal handler that jumps out must remove TSan's temporary
// all-blocked mask even when libc is not asked to restore a saved mask.

#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>

static sigjmp_buf checkpoint;
static pthread_t receiver;
static int ready;
static int sent;
static volatile sig_atomic_t handled;

static void handler(int sig) {
  if (sig == SIGUSR1) {
    ++handled;
    siglongjmp(checkpoint, 7);
  }
}

static void *sender(void *unused) {
  (void)unused;
  while (!__atomic_load_n(&ready, __ATOMIC_ACQUIRE)) {
  }
  int rc = pthread_kill(receiver, SIGUSR1);
  __atomic_store_n(&sent, rc ? -1 : 1, __ATOMIC_RELEASE);
  return NULL;
}

int main(void) {
  struct sigaction action = {0};
  action.sa_handler = handler;
  sigemptyset(&action.sa_mask);
  if (sigaction(SIGUSR1, &action, NULL))
    return 1;

  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR2);
  if (pthread_sigmask(SIG_SETMASK, &mask, NULL))
    return 2;
  receiver = pthread_self();

  for (int i = 0; i < 32; ++i) {
    pthread_t thread;
    __atomic_store_n(&ready, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&sent, 0, __ATOMIC_RELAXED);
    if (pthread_create(&thread, NULL, sender, NULL))
      return 3;
    int result = sigsetjmp(checkpoint, 0);
    if (!result) {
      __atomic_store_n(&ready, 1, __ATOMIC_RELEASE);
      struct timeval now;
      for (int spin = 0; spin < 10000000 &&
                         __atomic_load_n(&sent, __ATOMIC_ACQUIRE) >= 0;
           ++spin)
        gettimeofday(&now, NULL);
      return 4;
    }
    if (result != 7 || pthread_join(thread, NULL))
      return 5;
    if (pthread_sigmask(SIG_SETMASK, NULL, &mask) ||
        sigismember(&mask, SIGUSR1) != 0 ||
        sigismember(&mask, SIGUSR2) != 1)
      return 6;
  }
  if (handled != 32)
    return 7;
  puts("Darwin deferred siglongjmp mask restoration: OK");
  return 0;
}

// CHECK: Darwin deferred siglongjmp mask restoration: OK
