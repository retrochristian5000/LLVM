// RUN: %clangxx_tsan -O1 %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s

// Darwin's report of async-signal-unsafe allocation must not symbolize under
// the thread registry lock; both warnings and normal completion are required.

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t completed = 0;

static void Handler(int) {
  volatile char *buffer = (char *)malloc(16);
  if (buffer) {
    buffer[0] = 1;
    free((void *)buffer);
  }
  completed = 1;
}

int main() {
  struct sigaction action = {};
  action.sa_handler = Handler;
  sigemptyset(&action.sa_mask);
  if (sigaction(SIGUSR1, &action, nullptr) != 0)
    return 1;
  if (kill(getpid(), SIGUSR1) != 0 || completed != 1)
    return 2;
  static const char message[] = "Darwin signal-unsafe allocator: OK\n";
  write(STDOUT_FILENO, message, sizeof(message) - 1);
  return 0;
}

// CHECK: WARNING: ThreadSanitizer: signal-unsafe call inside of a signal
// CHECK: WARNING: ThreadSanitizer: signal-unsafe call inside of a signal
// CHECK: Darwin signal-unsafe allocator: OK
