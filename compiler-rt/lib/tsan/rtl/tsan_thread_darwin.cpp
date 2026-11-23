//===-- tsan_thread_darwin.cpp --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Darwin pthread TLS and lifecycle integration. Thread registry and fiber
// semantics remain in the platform-neutral tsan_thread.cpp.

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE

#include <limits.h>
#include <pthread.h>
#include <sys/mman.h>

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_mac.h"
#include "sanitizer_common/sanitizer_posix.h"
#include "tsan_platform.h"
#include "tsan_rtl.h"

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
void InitializeDarwinThreadSupport() {
  InitializeThreadStateStorage();

  ThreadEventCallbacks callbacks = {
      .create = ThreadCreateCallback,
      .start = nullptr,
      .terminate = ThreadTerminateCallback,
      .destroy = nullptr,
  };
  InstallPthreadIntrospectionHook(callbacks);
}

#endif  // !SANITIZER_GO

}  // namespace __tsan

#endif  // SANITIZER_APPLE
