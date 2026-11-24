//===-- tsan_cxx_darwin.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Darwin libc++ synchronization interceptors.
// These wrap dynamically-linked libc++ reference-counting and call_once
// operations so their synchronization is visible to ThreadSanitizer.
// Objective-C runtime synchronization lives in tsan_interceptors_objc.cpp.
//
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE && !SANITIZER_GO

#include "interception/interception.h"
#include "../rtl/tsan_interceptors.h"
#include "../interface/tsan_interface.h"
#include "../interface/tsan_interface_ann.h"

namespace __tsan {

// On macOS, libc++ is always linked dynamically, so intercepting works the
// usual way.
#  define STDCXX_INTERCEPTOR TSAN_INTERCEPTOR

namespace {
struct fake_shared_weak_count {
  volatile a64 shared_owners;
  volatile a64 shared_weak_owners;
  virtual void _unused_0x0() = 0;
  virtual void _unused_0x8() = 0;
  virtual void on_zero_shared() = 0;
  virtual void _unused_0x18() = 0;
  virtual void on_zero_shared_weak() = 0;
  virtual ~fake_shared_weak_count() = 0;  // suppress -Wnon-virtual-dtor
};
}  // namespace

// The following code adds libc++ interceptors for:
//     void __shared_weak_count::__release_shared() _NOEXCEPT;
//     bool __shared_count::__release_shared() _NOEXCEPT;
// Shared and weak pointers in C++ maintain reference counts via atomics in
// libc++.dylib, which are TSan-invisible, and this leads to false positives in
// destructor code. These interceptors re-implements the whole functions so that
// the mo_acq_rel semantics of the atomic decrement are visible.
//
// Unfortunately, the interceptors cannot simply Acquire/Release some sync
// object and call the original function, because it would have a race between
// the sync and the destruction of the object.  Calling both under a lock will
// not work because the destructor can invoke this interceptor again (and even
// in a different thread, so recursive locks don't help).

STDCXX_INTERCEPTOR(void, _ZNSt3__119__shared_weak_count16__release_sharedEv,
                   fake_shared_weak_count *o) {
  if (!flags()->shared_ptr_interceptor)
    return REAL(_ZNSt3__119__shared_weak_count16__release_sharedEv)(o);

  SCOPED_TSAN_INTERCEPTOR(_ZNSt3__119__shared_weak_count16__release_sharedEv,
                          o);
  if (__tsan_atomic64_fetch_add(&o->shared_owners, -1, mo_release) == 0) {
    Acquire(thr, pc, (uptr)&o->shared_owners);
    o->on_zero_shared();
    if (__tsan_atomic64_fetch_add(&o->shared_weak_owners, -1, mo_release) ==
        0) {
      Acquire(thr, pc, (uptr)&o->shared_weak_owners);
      o->on_zero_shared_weak();
    }
  }
}

STDCXX_INTERCEPTOR(bool, _ZNSt3__114__shared_count16__release_sharedEv,
                   fake_shared_weak_count *o) {
  if (!flags()->shared_ptr_interceptor)
    return REAL(_ZNSt3__114__shared_count16__release_sharedEv)(o);

  SCOPED_TSAN_INTERCEPTOR(_ZNSt3__114__shared_count16__release_sharedEv, o);
  if (__tsan_atomic64_fetch_add(&o->shared_owners, -1, mo_release) == 0) {
    Acquire(thr, pc, (uptr)&o->shared_owners);
    o->on_zero_shared();
    return true;
  }
  return false;
}

namespace {
struct call_once_callback_args {
  void (*orig_func)(void *arg);
  void *orig_arg;
  void *flag;
};

void call_once_callback_wrapper(void *arg) {
  call_once_callback_args *new_args = (call_once_callback_args *)arg;
  new_args->orig_func(new_args->orig_arg);
  __tsan_release(new_args->flag);
}
}  // namespace

// This adds a libc++ interceptor for:
//     void __call_once(volatile unsigned long&, void*, void(*)(void*));
// C++11 call_once is implemented via an internal function __call_once which is
// inside libc++.dylib, and the atomic release store inside it is thus
// TSan-invisible. To avoid false positives, this interceptor wraps the callback
// function and performs an explicit Release after the user code has run.
STDCXX_INTERCEPTOR(void, _ZNSt3__111__call_onceERVmPvPFvS2_E, void *flag,
                   void *arg, void (*func)(void *arg)) {
  call_once_callback_args new_args = {func, arg, flag};
  REAL(_ZNSt3__111__call_onceERVmPvPFvS2_E)(flag, &new_args,
                                            call_once_callback_wrapper);
}

#  undef STDCXX_INTERCEPTOR

}  // namespace __tsan

#endif  // SANITIZER_APPLE && !SANITIZER_GO
