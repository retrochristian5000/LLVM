// RUN: %clang_tsan -O1 %s -o %t && %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
// RUN: %clang_tsan -O2 %s -o %t.opt && %run %t.opt 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
//
// Darwin must invoke both callback signatures with their original function
// pointer types, including authenticated indirect calls on arm64e.

#include <signal.h>
#include <stdio.h>

static volatile sig_atomic_t plain_calls;
static volatile sig_atomic_t info_calls;
static volatile sig_atomic_t bad_context;

static void plain_handler(int signo) {
  if (signo == SIGUSR1)
    ++plain_calls;
}

static void info_handler(int signo, siginfo_t *info, void *context) {
  if (signo != SIGUSR1 || !info || info->si_signo != signo || !context)
    bad_context = 1;
  else
    ++info_calls;
}

int main(void) {
  struct sigaction action = {0};
  struct sigaction old = {0};
  sigemptyset(&action.sa_mask);

  for (int i = 0; i < 32; ++i) {
    action.sa_flags = 0;
    action.sa_handler = plain_handler;
    if (sigaction(SIGUSR1, &action, NULL) || raise(SIGUSR1))
      return 1;
    if (sigaction(SIGUSR1, NULL, &old) ||
        (old.sa_flags & SA_SIGINFO) || old.sa_handler != plain_handler)
      return 2;

    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = info_handler;
    if (sigaction(SIGUSR1, &action, &old) ||
        (old.sa_flags & SA_SIGINFO) || old.sa_handler != plain_handler ||
        raise(SIGUSR1))
      return 3;
    if (sigaction(SIGUSR1, NULL, &old) ||
        !(old.sa_flags & SA_SIGINFO) || old.sa_sigaction != info_handler)
      return 4;

    action.sa_flags = 0;
    action.sa_handler = SIG_IGN;
    if (sigaction(SIGUSR1, &action, &old) ||
        !(old.sa_flags & SA_SIGINFO) || old.sa_sigaction != info_handler ||
        raise(SIGUSR1))
      return 5;
  }

  if (plain_calls != 32 || info_calls != 32 || bad_context)
    return 6;
  puts("Darwin signal handler ABI: OK");
  return 0;
}

// CHECK: Darwin signal handler ABI: OK
