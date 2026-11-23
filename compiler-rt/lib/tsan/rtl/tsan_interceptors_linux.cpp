//===-- tsan_interceptors_linux.cpp ---------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Linux-specific file-descriptor and epoll interceptors. Shared POSIX
// interceptors and signal dispatch remain in tsan_interceptors_posix.cpp.

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_LINUX

#include "interception/interception.h"
#include "sanitizer_common/sanitizer_errno.h"
#include "tsan_fd.h"
#include "tsan_interceptors.h"
#include "tsan_rtl.h"

using namespace __tsan;

// Linux UAPI: EPOLL_CTL_ADD is operation 1 for epoll_ctl.
static constexpr int kEpollCtlAdd = 1;

TSAN_INTERCEPTOR(int, eventfd, unsigned initval, int flags) {
  SCOPED_TSAN_INTERCEPTOR(eventfd, initval, flags);
  int fd = REAL(eventfd)(initval, flags);
  if (fd >= 0)
    FdEventCreate(thr, pc, fd);
  return fd;
}

TSAN_INTERCEPTOR(int, signalfd, int fd, void *mask, int flags) {
  SCOPED_INTERCEPTOR_RAW(signalfd, fd, mask, flags);
  FdClose(thr, pc, fd);
  fd = REAL(signalfd)(fd, mask, flags);
  if (!MustIgnoreInterceptor(thr))
    FdSignalCreate(thr, pc, fd);
  return fd;
}

TSAN_INTERCEPTOR(int, inotify_init, int fake) {
  SCOPED_TSAN_INTERCEPTOR(inotify_init, fake);
  int fd = REAL(inotify_init)(fake);
  if (fd >= 0)
    FdInotifyCreate(thr, pc, fd);
  return fd;
}

TSAN_INTERCEPTOR(int, inotify_init1, int flags) {
  SCOPED_TSAN_INTERCEPTOR(inotify_init1, flags);
  int fd = REAL(inotify_init1)(flags);
  if (fd >= 0)
    FdInotifyCreate(thr, pc, fd);
  return fd;
}

TSAN_INTERCEPTOR(int, epoll_create, int size) {
  SCOPED_TSAN_INTERCEPTOR(epoll_create, size);
  int fd = REAL(epoll_create)(size);
  if (fd >= 0)
    FdPollCreate(thr, pc, fd);
  return fd;
}

TSAN_INTERCEPTOR(int, epoll_create1, int flags) {
  SCOPED_TSAN_INTERCEPTOR(epoll_create1, flags);
  int fd = REAL(epoll_create1)(flags);
  if (fd >= 0)
    FdPollCreate(thr, pc, fd);
  return fd;
}

TSAN_INTERCEPTOR(int, epoll_ctl, int epfd, int op, int fd, void *ev) {
  SCOPED_TSAN_INTERCEPTOR(epoll_ctl, epfd, op, fd, ev);
  if (epfd >= 0)
    FdAccess(thr, pc, epfd);
  if (epfd >= 0 && fd >= 0)
    FdAccess(thr, pc, fd);
  if (op == kEpollCtlAdd && epfd >= 0) {
    FdPollAdd(thr, pc, epfd, fd);
    FdRelease(thr, pc, epfd);
  }
  int res = REAL(epoll_ctl)(epfd, op, fd, ev);
  return res;
}

TSAN_INTERCEPTOR(int, epoll_wait, int epfd, void *ev, int cnt, int timeout) {
  SCOPED_TSAN_INTERCEPTOR(epoll_wait, epfd, ev, cnt, timeout);
  if (epfd >= 0)
    FdAccess(thr, pc, epfd);
  int res = BLOCK_REAL(epoll_wait)(epfd, ev, cnt, timeout);
  if (res > 0 && epfd >= 0)
    FdAcquire(thr, pc, epfd);
  return res;
}

TSAN_INTERCEPTOR(int, epoll_pwait, int epfd, void *ev, int cnt, int timeout,
                 void *sigmask) {
  SCOPED_TSAN_INTERCEPTOR(epoll_pwait, epfd, ev, cnt, timeout, sigmask);
  if (epfd >= 0)
    FdAccess(thr, pc, epfd);
  int res = BLOCK_REAL(epoll_pwait)(epfd, ev, cnt, timeout, sigmask);
  if (res > 0 && epfd >= 0)
    FdAcquire(thr, pc, epfd);
  return res;
}

TSAN_INTERCEPTOR(int, epoll_pwait2, int epfd, void *ev, int cnt, void *timeout,
                 void *sigmask) {
  SCOPED_INTERCEPTOR_RAW(epoll_pwait2, epfd, ev, cnt, timeout, sigmask);
  // This function is new and may not be present in libc and/or kernel.
  // Since we effectively add it to libc (as will be probed by the program
  // using dlsym or a weak function pointer) we need to handle the case
  // when it's not present in the actual libc.
  if (!REAL(epoll_pwait2)) {
    errno = errno_ENOSYS;
    return -1;
  }
  if (MustIgnoreInterceptor(thr))
    return REAL(epoll_pwait2)(epfd, ev, cnt, timeout, sigmask);
  if (epfd >= 0)
    FdAccess(thr, pc, epfd);
  int res = BLOCK_REAL(epoll_pwait2)(epfd, ev, cnt, timeout, sigmask);
  if (res > 0 && epfd >= 0)
    FdAcquire(thr, pc, epfd);
  return res;
}

namespace __tsan {

void InitializeLinuxInterceptors() {
  INTERCEPT_FUNCTION(eventfd);
  INTERCEPT_FUNCTION(signalfd);
  INTERCEPT_FUNCTION(inotify_init);
  INTERCEPT_FUNCTION(inotify_init1);
  INTERCEPT_FUNCTION(epoll_create);
  INTERCEPT_FUNCTION(epoll_create1);
  INTERCEPT_FUNCTION(epoll_ctl);
  INTERCEPT_FUNCTION(epoll_wait);
  INTERCEPT_FUNCTION(epoll_pwait);
  INTERCEPT_FUNCTION(epoll_pwait2);
}

}  // namespace __tsan

#endif  // SANITIZER_LINUX
