//===-- tsan_platform_mac.cpp ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
// Mac-specific code.
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"
#if SANITIZER_APPLE

#include "sanitizer_common/sanitizer_atomic.h"
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_libc.h"
#include "sanitizer_common/sanitizer_posix.h"
#include "sanitizer_common/sanitizer_procmaps.h"
#include "tsan_platform.h"
#include "tsan_rtl.h"
#include "tsan_flags.h"

#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>

namespace __tsan {

#if !SANITIZER_GO
alignas(SANITIZER_CACHE_LINE_SIZE) static char main_thread_state[sizeof(
    ThreadState)];
static ThreadState *dead_thread_state;
static pthread_key_t thread_state_key;

// We rely on the following documented, but Darwin-specific behavior to keep the
// reference to the ThreadState object alive in TLS:
// pthread_key_create man page:
//   If, after all the destructors have been called for all non-NULL values with
//   associated destructors, there are still some non-NULL values with
//   associated destructors, then the process is repeated.  If, after at least
//   [PTHREAD_DESTRUCTOR_ITERATIONS] iterations of destructor calls for
//   outstanding non-NULL values, there are still some non-NULL values with
//   associated destructors, the implementation stops calling destructors.
static_assert(PTHREAD_DESTRUCTOR_ITERATIONS == 4, "Small number of iterations");
static void ThreadStateDestructor(void *thr) {
  int res = pthread_setspecific(thread_state_key, thr);
  CHECK_EQ(res, 0);
}

static void InitializeThreadStateStorage() {
  int res;
  CHECK_EQ(thread_state_key, 0);
  res = pthread_key_create(&thread_state_key, ThreadStateDestructor);
  CHECK_EQ(res, 0);
  res = pthread_setspecific(thread_state_key, main_thread_state);
  CHECK_EQ(res, 0);

  auto dts = (ThreadState *)MmapOrDie(sizeof(ThreadState), "ThreadState");
  dts->fast_state.SetIgnoreBit();
  dts->ignore_interceptors = 1;
  dts->is_dead = true;
  const_cast<Tid &>(dts->tid) = kInvalidTid;
  res = internal_mprotect(dts, sizeof(ThreadState), PROT_READ);  // immutable
  CHECK_EQ(res, 0);
  dead_thread_state = dts;
}

ThreadState *cur_thread() {
  // Some interceptors get called before libpthread has been initialized and in
  // these cases we must avoid calling any pthread APIs.
  if (UNLIKELY(!thread_state_key)) {
    return (ThreadState *)main_thread_state;
  }

  // We only reach this line after InitializeThreadStateStorage() ran, i.e,
  // after TSan (and therefore libpthread) have been initialized.
  ThreadState *thr = (ThreadState *)pthread_getspecific(thread_state_key);
  if (UNLIKELY(!thr)) {
    thr = (ThreadState *)MmapOrDie(sizeof(ThreadState), "ThreadState");
    int res = pthread_setspecific(thread_state_key, thr);
    CHECK_EQ(res, 0);
  }
  return thr;
}

void set_cur_thread(ThreadState *thr) {
  int res = pthread_setspecific(thread_state_key, thr);
  CHECK_EQ(res, 0);
}

void cur_thread_finalize() {
  ThreadState *thr = (ThreadState *)pthread_getspecific(thread_state_key);
  CHECK(thr);
  if (thr == (ThreadState *)main_thread_state) {
    // Calling dispatch_main() or xpc_main() actually invokes pthread_exit to
    // exit the main thread. Let's keep the main thread's ThreadState.
    return;
  }
  // Intercepted functions can still get called after cur_thread_finalize()
  // (called from DestroyThreadState()), so put a fake thread state for "dead"
  // threads.  An alternative solution would be to release the ThreadState
  // object from THREAD_DESTROY (which is delivered later and on the parent
  // thread) instead of THREAD_TERMINATE.
  int res = pthread_setspecific(thread_state_key, dead_thread_state);
  CHECK_EQ(res, 0);
  UnmapOrDie(thr, sizeof(ThreadState));
}
#endif

#  if !SANITIZER_GO
void InitializeShadowMemoryPlatform() { }

// Register GCD worker threads, which are created without an observable call to
// pthread_create().
static void ThreadCreateCallback(uptr thread, bool gcd_worker) {
  if (gcd_worker) {
    ThreadState *thr = cur_thread();
    Processor *proc = ProcCreate();
    ProcWire(proc, thr);
    ThreadState *parent_thread_state = nullptr;  // No parent.
    Tid tid = ThreadCreate(parent_thread_state, 0, (uptr)thread, true);
    CHECK_NE(tid, kMainTid);
    ThreadStart(thr, tid, GetTid(), ThreadType::Worker);
  }
}

// Destroy thread state for *all* threads.
static void ThreadTerminateCallback(uptr thread) {
  ThreadState *thr = cur_thread();
  if (thr->tctx) {
    DestroyThreadState();
  }
}
#endif

void InitializePlatformEarly() {
#  if !SANITIZER_GO && SANITIZER_IOS
  uptr max_vm = GetMaxUserVirtualAddress() + 1;
  if (max_vm < HiAppMemEnd()) {
    Report(
        "ThreadSanitizer: Unsupported virtual memory layout:\n\tVM address "
        "limit = %p\n\tExpected %p.\n",
        (void*)max_vm, (void*)HiAppMemEnd());
    Die();
  }
  // In some configurations, the max_vm is expanded, but much of this space is
  // already mapped. TSAN will not work in this configuration.
  if (!MemoryRangeIsAvailable(HiAppMemEnd() - 1, HiAppMemEnd() - 1)) {
    Report(
        "ThreadSanitizer: Unsupported virtual memory layout: Address %p is "
        "already mapped.\n",
        (void*)(HiAppMemEnd() - 1));
    Die();
  }
#endif
}

void InitializePlatform() {
  DisableCoreDumperIfNecessary();
#if !SANITIZER_GO
  if (!CheckAndProtect(true, true, true)) {
    Printf("FATAL: ThreadSanitizer: found incompatible memory layout.\n");
    Die();
  }

  InitializeThreadStateStorage();

  ThreadEventCallbacks callbacks = {
      .create = ThreadCreateCallback,
      .start = nullptr,
      .terminate = ThreadTerminateCallback,
      .destroy = nullptr,
  };
  InstallPthreadIntrospectionHook(callbacks);
#endif

  InitializeDarwinLongJmp();
}


#if !SANITIZER_GO
extern "C" void __tsan_tls_initialization() {}

void ImitateTlsWrite(ThreadState *thr, uptr tls_addr, uptr tls_size) {
  const uptr pc = StackTrace::GetNextInstructionPc(
      reinterpret_cast<uptr>(__tsan_tls_initialization));
  // Unlike Linux, we only store a pointer to the ThreadState object in TLS;
  // just mark the entire range as written to.
  MemoryRangeImitateWrite(thr, pc, tls_addr, tls_size);
}
#endif

#if !SANITIZER_GO
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
#endif

}  // namespace __tsan

#endif  // SANITIZER_APPLE
