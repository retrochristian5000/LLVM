//===-- tsan_interceptors_freebsd.cpp -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// FreeBSD libthr aliases and thread-exit interception. The public POSIX
// pthread interceptors live in tsan_interceptors_posix.cpp.

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_FREEBSD

#include "interception/interception.h"
#include "sanitizer_common/sanitizer_platform_limits_posix.h"
#include "tsan_interceptors.h"
#include "tsan_platform.h"

using namespace __tsan;

TSAN_INTERCEPTOR(void, thr_exit, ThreadID *state) {
  SCOPED_TSAN_INTERCEPTOR(thr_exit, state);
  DestroyThreadState();
  REAL(thr_exit)(state);
}

// GCC/Clang alias attributes require the alias target in the same translation
// unit. The POSIX wrappers are defined in another source, so forward to them
// instead of attaching cross-TU aliases. Do not call the real pthread functions
// here: that would bypass TSan synchronization bookkeeping.
#define TSAN_FREEBSD_EXPAND_ARGS(...) __VA_ARGS__
#define TSAN_FREEBSD_FORWARD_ALIAS(ret, name, signature, args) \
  extern "C" ret WRAP(pthread_##name) signature;                \
  TSAN_INTERCEPTOR(ret, _pthread_##name, TSAN_FREEBSD_EXPAND_ARGS signature) { \
    return WRAP(pthread_##name) args;                            \
  }

TSAN_FREEBSD_FORWARD_ALIAS(int, cond_init, (void *cond, void *attr), (cond, attr))
TSAN_FREEBSD_FORWARD_ALIAS(int, cond_destroy, (void *cond), (cond))
TSAN_FREEBSD_FORWARD_ALIAS(int, cond_signal, (void *cond), (cond))
TSAN_FREEBSD_FORWARD_ALIAS(int, cond_broadcast, (void *cond), (cond))
TSAN_FREEBSD_FORWARD_ALIAS(int, cond_wait, (void *cond, void *mutex), (cond, mutex))
TSAN_FREEBSD_FORWARD_ALIAS(int, mutex_init, (void *mutex, void *attr), (mutex, attr))
TSAN_FREEBSD_FORWARD_ALIAS(int, mutex_destroy, (void *mutex), (mutex))
TSAN_FREEBSD_FORWARD_ALIAS(int, mutex_lock, (void *mutex), (mutex))
TSAN_FREEBSD_FORWARD_ALIAS(int, mutex_trylock, (void *mutex), (mutex))
TSAN_FREEBSD_FORWARD_ALIAS(int, mutex_unlock, (void *mutex), (mutex))
TSAN_FREEBSD_FORWARD_ALIAS(int, rwlock_init, (void *lock, void *attr), (lock, attr))
TSAN_FREEBSD_FORWARD_ALIAS(int, rwlock_destroy, (void *lock), (lock))
TSAN_FREEBSD_FORWARD_ALIAS(int, rwlock_rdlock, (void *lock), (lock))
TSAN_FREEBSD_FORWARD_ALIAS(int, rwlock_tryrdlock, (void *lock), (lock))
TSAN_FREEBSD_FORWARD_ALIAS(int, rwlock_wrlock, (void *lock), (lock))
TSAN_FREEBSD_FORWARD_ALIAS(int, rwlock_trywrlock, (void *lock), (lock))
TSAN_FREEBSD_FORWARD_ALIAS(int, rwlock_unlock, (void *lock), (lock))
TSAN_FREEBSD_FORWARD_ALIAS(int, once, (void *once_control, void (*init_routine)()), (once_control, init_routine))
TSAN_FREEBSD_FORWARD_ALIAS(int, sigmask, (int how, const __sanitizer_sigset_t *set, __sanitizer_sigset_t *oldset), (how, set, oldset))

#undef TSAN_FREEBSD_FORWARD_ALIAS
#undef TSAN_FREEBSD_EXPAND_ARGS

namespace __tsan {

void InitializeFreeBSDInterceptors() {
  INTERCEPT_FUNCTION(thr_exit);
  INTERCEPT_FUNCTION(_pthread_cond_init);
  INTERCEPT_FUNCTION(_pthread_cond_destroy);
  INTERCEPT_FUNCTION(_pthread_cond_signal);
  INTERCEPT_FUNCTION(_pthread_cond_broadcast);
  INTERCEPT_FUNCTION(_pthread_cond_wait);
  INTERCEPT_FUNCTION(_pthread_mutex_init);
  INTERCEPT_FUNCTION(_pthread_mutex_destroy);
  INTERCEPT_FUNCTION(_pthread_mutex_lock);
  INTERCEPT_FUNCTION(_pthread_mutex_trylock);
  INTERCEPT_FUNCTION(_pthread_mutex_unlock);
  INTERCEPT_FUNCTION(_pthread_rwlock_init);
  INTERCEPT_FUNCTION(_pthread_rwlock_destroy);
  INTERCEPT_FUNCTION(_pthread_rwlock_rdlock);
  INTERCEPT_FUNCTION(_pthread_rwlock_tryrdlock);
  INTERCEPT_FUNCTION(_pthread_rwlock_wrlock);
  INTERCEPT_FUNCTION(_pthread_rwlock_trywrlock);
  INTERCEPT_FUNCTION(_pthread_rwlock_unlock);
  INTERCEPT_FUNCTION(_pthread_once);
  INTERCEPT_FUNCTION(_pthread_sigmask);
}

}  // namespace __tsan

#endif  // SANITIZER_FREEBSD
