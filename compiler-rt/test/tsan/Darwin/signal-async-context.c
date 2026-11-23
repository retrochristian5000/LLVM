// RUN: %clang_tsan -O1 -pthread %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
//
// An asynchronous signal can be deferred by TSan and replayed through its
// SignalDesc. Darwin may supply a null third argument to a signal handler;
// forwarding must preserve that value rather than dereference it.

#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>

static int ready;
static volatile sig_atomic_t handled;

static void handler(int sig, siginfo_t *info, void *ctx) {
  (void)ctx; // Either a real context or null is valid.
  if (sig == SIGUSR1 && info)
    handled = 1;
}

static void *worker(void *unused) {
  (void)unused;
  __atomic_store_n(&ready, 1, __ATOMIC_RELEASE);
  for (int i = 0; i < 100000 && !handled; ++i) {
    struct timeval tv;
    // This intercepted call gives TSan a chance to process deferred signals.
    gettimeofday(&tv, 0);
  }
  return handled ? 0 : (void *)1;
}

int main(void) {
  struct sigaction action = {0};
  action.sa_sigaction = handler;
  action.sa_flags = SA_SIGINFO;
  sigemptyset(&action.sa_mask);
  if (sigaction(SIGUSR1, &action, 0) != 0)
    return 1;

  pthread_t thread;
  if (pthread_create(&thread, 0, worker, 0) != 0)
    return 1;
  while (!__atomic_load_n(&ready, __ATOMIC_ACQUIRE))
    sched_yield();

  if (pthread_kill(thread, SIGUSR1) != 0)
    return 1;
  void *result;
  if (pthread_join(thread, &result) != 0 || result != 0)
    return 1;

  puts("asynchronous signal context: OK");
  return 0;
}

// CHECK: asynchronous signal context: OK
