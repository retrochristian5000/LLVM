// RUN: %clangxx_tsan -O1 %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'

// Darwin kqueue() returns a descriptor and kevent() registers and receives
// readiness notifications. No unrelated synchronization edges are inferred.

#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

int main() {
  int fd[2];
  if (pipe(fd) != 0)
    return 1;

  int kq = kqueue();
  if (kq < 0)
    return 2;

  struct kevent change;
  EV_SET(&change, fd[0], EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
  if (kevent(kq, &change, 1, nullptr, 0, nullptr) != 0)
    return 3;

  char byte = 'x';
  if (write(fd[1], &byte, 1) != 1)
    return 4;

  struct timespec timeout = {1, 0};
  struct kevent received;
  if (kevent(kq, nullptr, 0, &received, 1, &timeout) != 1 ||
      received.ident != (uintptr_t)fd[0] ||
      received.filter != EVFILT_READ)
    return 5;

  if (read(fd[0], &byte, 1) != 1 ||
      close(kq) != 0 || close(fd[0]) != 0 || close(fd[1]) != 0)
    return 6;

  puts("Darwin kqueue fd interception: OK");
  return 0;
}

// CHECK: Darwin kqueue fd interception: OK
