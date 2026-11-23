//===-- tsan_interceptors_glibc.cpp ---------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// glibc-specific interceptors. Linux kernel interfaces are handled by
// tsan_interceptors_linux.cpp; POSIX interfaces stay in the common unit.
// Building this source on a non-glibc Linux target must not add interceptors.

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_GLIBC

#include "interception/interception.h"
#include "sanitizer_common/sanitizer_errno.h"
#include "sanitizer_common/sanitizer_glibc_version.h"
#include "tsan_adaptive_delay.h"
#include "tsan_fd.h"
#include "tsan_interceptors.h"
#include "tsan_platform.h"

using namespace __tsan;

#  if !__GLIBC_PREREQ(2, 34)
// glibc 2.34 applies a non-default version for the two functions. They are no
// longer expected to be intercepted by programs.
TSAN_INTERCEPTOR(int, __pthread_mutex_lock, void *m) {
  SCOPED_TSAN_INTERCEPTOR(__pthread_mutex_lock, m);
  MutexPreLock(thr, pc, (uptr)m);
  AdaptiveDelay::SyncOp();
  int res = BLOCK_REAL(__pthread_mutex_lock)(m);
  if (res == errno_EOWNERDEAD)
    MutexRepair(thr, pc, (uptr)m);
  if (res == 0 || res == errno_EOWNERDEAD)
    MutexPostLock(thr, pc, (uptr)m);
  if (res == errno_EINVAL)
    MutexInvalidAccess(thr, pc, (uptr)m);
  return res;
}

TSAN_INTERCEPTOR(int, __pthread_mutex_unlock, void *m) {
  SCOPED_TSAN_INTERCEPTOR(__pthread_mutex_unlock, m);
  MutexUnlock(thr, pc, (uptr)m);
  int res = REAL(__pthread_mutex_unlock)(m);
  AdaptiveDelay::SyncOp();
  if (res == errno_EINVAL)
    MutexInvalidAccess(thr, pc, (uptr)m);
  return res;
}
#  endif

TSAN_INTERCEPTOR(int, __fxstat, int version, int fd, void *buf) {
  SCOPED_TSAN_INTERCEPTOR(__fxstat, version, fd, buf);
  if (fd > 0)
    FdAccess(thr, pc, fd);
  return REAL(__fxstat)(version, fd, buf);
}

TSAN_INTERCEPTOR(int, __fxstat64, int version, int fd, void *buf) {
  SCOPED_TSAN_INTERCEPTOR(__fxstat64, version, fd, buf);
  if (fd > 0)
    FdAccess(thr, pc, fd);
  return REAL(__fxstat64)(version, fd, buf);
}

// Resolver cleanup is a glibc internal implementation detail, not POSIX.
TSAN_INTERCEPTOR(void, __res_iclose, void *state, bool free_addr) {
  SCOPED_INTERCEPTOR_RAW(__res_iclose, state, free_addr);
  int fds[64];
  int cnt = ExtractResolvFDs(state, fds, ARRAY_SIZE(fds));
  for (int i = 0; i < cnt; i++) FdClose(thr, pc, fds[i]);
  REAL(__res_iclose)(state, free_addr);
}

namespace __tsan {

void InitializeGlibcInterceptors() {
#if !__GLIBC_PREREQ(2, 34)
  INTERCEPT_FUNCTION(__pthread_mutex_lock);
  INTERCEPT_FUNCTION(__pthread_mutex_unlock);
#endif
  INTERCEPT_FUNCTION(__fxstat);
  INTERCEPT_FUNCTION(__fxstat64);
  INTERCEPT_FUNCTION(__res_iclose);
}

}  // namespace __tsan

#endif  // SANITIZER_GLIBC
