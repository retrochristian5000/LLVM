// RUN: %clang_tsan -O1 %s -o %t && %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'

#include <errno.h>
#include <signal.h>
#include <stdio.h>

static volatile sig_atomic_t handled;

static void handler(int sig) {
  if (sig == SIGUSR1)
    ++handled;
}

int main(void) {
  struct sigaction action = {0};
  action.sa_handler = handler;
  sigemptyset(&action.sa_mask);
  const int invalid[] = {0, -1, NSIG, NSIG + 1, SIGKILL, SIGSTOP};
  for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
    errno = 0;
    if (sigaction(invalid[i], &action, 0) != -1 || errno != EINVAL)
      return 1;
  }
  if (sigaction(SIGUSR1, &action, 0) || raise(SIGUSR1) || handled != 1)
    return 2;
  puts("Darwin signal ABI boundaries: OK");
  return 0;
}

// CHECK: Darwin signal ABI boundaries: OK
