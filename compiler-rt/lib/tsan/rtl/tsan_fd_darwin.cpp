//===-- tsan_fd_darwin.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Darwin kernel-event queue file descriptor interception.
// The shared descriptor lifetime, metadata, and I/O synchronization model
// remain in tsan_fd.cpp. Do not infer happens-before edges from arbitrary
// kernel events: filters may represent timers, processes, or user triggers.

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE

#include <sys/event.h>
#include <time.h>

#include "interception/interception.h"
#include "tsan_fd.h"
#include "tsan_interceptors.h"

using namespace __tsan;

TSAN_INTERCEPTOR(int, kqueue, void) {
  SCOPED_TSAN_INTERCEPTOR(kqueue);
  int fd = REAL(kqueue)();
  if (fd >= 0)
    FdPollCreate(thr, pc, fd);
  return fd;
}

TSAN_INTERCEPTOR(int, kevent, int kq, const struct kevent *changes,
                 int nchanges, struct kevent *events, int nevents,
                 const struct timespec *timeout) {
  SCOPED_TSAN_INTERCEPTOR(kevent, kq, changes, nchanges, events, nevents,
                          timeout);
  if (kq >= 0)
    FdAccess(thr, pc, kq);
  // A returned event alone does not establish which user operation woke the
  // queue. Only observe descriptor use here; do not invent an Acquire.
  return BLOCK_REAL(kevent)(kq, changes, nchanges, events, nevents, timeout);
}

#endif  // SANITIZER_APPLE
