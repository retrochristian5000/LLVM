//===-- tsan_thread_posix.cpp ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// POSIX thread storage and cancellation support. Thread registry and fiber
// semantics remain in tsan_thread.cpp. Darwin-specific thread state and
// pthread introspection remain in tsan_thread_darwin.cpp.
//
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_POSIX && !SANITIZER_GO

#include <pthread.h>

#include "sanitizer_common/sanitizer_common.h"
#include "tsan_platform.h"
#include "tsan_rtl.h"

namespace __tsan {

#if !SANITIZER_APPLE
// Non-Apple POSIX targets retain the initial ThreadState in static TLS.
alignas(SANITIZER_CACHE_LINE_SIZE) THREADLOCAL __attribute__((tls_model(
    "initial-exec"))) char cur_thread_placeholder[sizeof(ThreadState)];
#endif

#if SANITIZER_LINUX || SANITIZER_FREEBSD || SANITIZER_NETBSD
extern "C" void __tsan_tls_initialization() {}

void ImitateTlsWrite(ThreadState *thr, uptr tls_addr, uptr tls_size) {
  // Check that the thr object is in tls;
  const uptr thr_beg = (uptr)thr;
  const uptr thr_end = (uptr)thr + sizeof(*thr);
  CHECK_GE(thr_beg, tls_addr);
  CHECK_LE(thr_beg, tls_addr + tls_size);
  CHECK_GE(thr_end, tls_addr);
  CHECK_LE(thr_end, tls_addr + tls_size);
  // Since the thr object is huge, skip it.
  const uptr pc = StackTrace::GetNextInstructionPc(
      reinterpret_cast<uptr>(__tsan_tls_initialization));
  MemoryRangeImitateWrite(thr, pc, tls_addr, thr_beg - tls_addr);
  MemoryRangeImitateWrite(thr, pc, thr_end, tls_addr + tls_size - thr_end);
}
#endif  // SANITIZER_LINUX || SANITIZER_FREEBSD || SANITIZER_NETBSD

// Note: this function runs with async signals enabled,
// so it must not touch any tsan state.
int call_pthread_cancel_with_cleanup(int (*fn)(void *arg),
                                     void (*cleanup)(void *arg), void *arg) {
  // pthread_cleanup_push/pop are hardcore macros mess.
  // We can't intercept nor call them w/o including pthread.h.
  int res;
  pthread_cleanup_push(cleanup, arg);
  res = fn(arg);
  pthread_cleanup_pop(0);
  return res;
}

}  // namespace __tsan

#endif  // SANITIZER_POSIX && !SANITIZER_GO
