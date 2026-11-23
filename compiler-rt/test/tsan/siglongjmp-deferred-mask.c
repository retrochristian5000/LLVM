// RUN: %clang_tsan -std=gnu11 -O1 -pthread %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'

// A siglongjmp out of TSan's deferred signal callback skips the callback's
// normal signal-mask restoration. TSan must undo its temporary mask even
// when sigsetjmp(env, 0) deliberately did not save the libc signal mask.

#include <pthread.h>
#include <sched.h>
#include <setjmp.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <sys/time.h>

static sigjmp_buf checkpoint;
static pthread_t victim;
static pthread_t sender;
static atomic_int sent;
static atomic_int start_sender;
static volatile sig_atomic_t handler_count;

static void on_usr1(int signo) {
  if (signo == SIGUSR1) {
    handler_count++;
    siglongjmp(checkpoint, 17);
  }
}

static void *send_usr1(void *unused) {
  (void)unused;
  while (!atomic_load_explicit(&start_sender, memory_order_acquire))
    sched_yield();
  int rc = pthread_kill(victim, SIGUSR1);
  atomic_store_explicit(&sent, rc ? -1 : 1, memory_order_release);
  return NULL;
}

static int run_case(int savemask) {
  sigset_t empty;
  sigemptyset(&empty);
  if (pthread_sigmask(SIG_SETMASK, &empty, NULL) != 0)
    return 1;

  atomic_store_explicit(&sent, 0, memory_order_relaxed);
  atomic_store_explicit(&start_sender, 0, memory_order_relaxed);
  int result = sigsetjmp(checkpoint, savemask);
  if (result == 0) {
    sigset_t usr2;
    sigemptyset(&usr2);
    sigaddset(&usr2, SIGUSR2);
    if (pthread_sigmask(SIG_BLOCK, &usr2, NULL) != 0)
      return 2;
    victim = pthread_self();
    if (pthread_create(&sender, NULL, send_usr1, NULL) != 0)
      return 3;

    atomic_store_explicit(&start_sender, 1, memory_order_release);
    // Intercepted gettimeofday drives TSan's deferred-signal replay.
    struct timeval tv;
    for (int i = 0; i < 10000000; ++i)
      gettimeofday(&tv, NULL);
    return 4;
  }

  if (result != 17)
    return 5;
  if (pthread_join(sender, NULL) != 0)
    return 6;
  if (atomic_load_explicit(&sent, memory_order_acquire) != 1)
    return 7;

  sigset_t mask;
  if (pthread_sigmask(SIG_SETMASK, NULL, &mask) != 0)
    return 8;
  // SIGTERM is unrelated to the delivered signal and must remain unblocked.
  if (sigismember(&mask, SIGTERM) != 0)
    return 9;
  // libc restores the saved mask only for sigsetjmp(savemask != 0).
  if (sigismember(&mask, SIGUSR2) != (savemask ? 0 : 1))
    return 10;
  return 0;
}

int main(void) {
  struct sigaction sa = {0};
  sa.sa_handler = on_usr1;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGUSR1, &sa, NULL) != 0)
    return 1;

  int rc = run_case(0);
  if (rc != 0) {
    fprintf(stderr, "savemask=0 failed: %d\n", rc);
    return 1;
  }
  rc = run_case(1);
  if (rc != 0) {
    fprintf(stderr, "savemask=1 failed: %d\n", rc);
    return 1;
  }
  if (handler_count != 2)
    return 1;
  puts("deferred siglongjmp mask: OK");
  return 0;
}

// CHECK: deferred siglongjmp mask: OK
