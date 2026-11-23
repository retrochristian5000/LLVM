// RUN: %clang_tsan -D_GNU_SOURCE -O1 -pthread %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
// REQUIRES: linux

// The Linux-specific interceptor unit must register eventfd, signalfd,
// inotify, and epoll without changing their ordinary system-call behavior.

#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/signalfd.h>
#include <unistd.h>

int main(void) {
  int ep = epoll_create1(EPOLL_CLOEXEC);
  int ev = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (ep < 0 || ev < 0)
    return 1;

  struct epoll_event entry = {0};
  entry.events = EPOLLIN;
  entry.data.fd = ev;
  if (epoll_ctl(ep, EPOLL_CTL_ADD, ev, &entry) != 0)
    return 2;

  const uint64_t written = 1;
  if (write(ev, &written, sizeof(written)) != (ssize_t)sizeof(written))
    return 3;

  struct epoll_event ready = {0};
  if (epoll_wait(ep, &ready, 1, 1000) != 1 || ready.data.fd != ev)
    return 4;
  uint64_t received = 0;
  if (read(ev, &received, sizeof(received)) != (ssize_t)sizeof(received) ||
      received != written)
    return 5;

  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR2);
  if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0)
    return 6;
  int sigfd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
  if (sigfd < 0)
    return 7;
  int notify = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
  if (notify < 0)
    return 8;

  close(notify);
  close(sigfd);
  if (pthread_sigmask(SIG_UNBLOCK, &mask, NULL) != 0)
    return 9;
  close(ev);
  close(ep);

  puts("Linux fd interceptors: OK");
  return 0;
}

// CHECK: Linux fd interceptors: OK
