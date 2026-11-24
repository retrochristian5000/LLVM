//===-- tsan_interceptors_darwin.cpp --------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Native Darwin signal ABI glue used by the shared POSIX signal dispatcher.
// This does not register a second set of interceptors.

#include "tsan_interceptors_darwin.h"

#if SANITIZER_APPLE

#include <pthread.h>
#include <signal.h>
#include <sys/ucontext.h>

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_libc.h"
#include "sanitizer_common/sanitizer_ptrauth.h"

namespace __tsan {

const int kDarwinSiginfoFlag = SA_SIGINFO;
const int kDarwinSigSetMask = SIG_SETMASK;
const int kDarwinSigTerm = SIGTERM;
const int kDarwinSigSys = SIGSYS;

bool IsDarwinSignalNumber(int sig) {
  return sig > 0 && sig < NSIG;
}

bool CanChangeDarwinSignalAction(int sig) {
  return IsDarwinSignalNumber(sig) && sig != SIGKILL && sig != SIGSTOP;
}

bool IsDarwinSynchronousSignal(int sig) {
  return sig == SIGSEGV || sig == SIGBUS || sig == SIGILL || sig == SIGTRAP ||
         sig == SIGABRT || sig == SIGFPE || sig == SIGPIPE || sig == SIGSYS;
}

bool IsDarwinRecursiveMutexType(int type) {
  return type == PTHREAD_MUTEX_RECURSIVE;
}

static_assert(sizeof(siginfo_t) <= 128,
              "Darwin siginfo_t exceeds POSIX interceptor storage");

uptr InvokeDarwinSignalHandler(const __sanitizer_sigaction &action, int sig,
                               __sanitizer_siginfo *info, void *uctx) {
  // The disposition can change concurrently. Read the selected callback
  // once and retain it as a typed function pointer for the authenticated call.
  const volatile __sanitizer_sigaction &current = action;
  if (current.sa_flags & SA_SIGINFO) {
    const auto handler = current.sigaction;
    const uptr value = reinterpret_cast<uptr>(handler);
    if (value == sig_dfl || value == sig_ign)
      return value;
    using NativeHandler = void (*)(int, siginfo_t *, void *);
    const uptr pc = STRIP_PAC_PC(handler);
    reinterpret_cast<NativeHandler>(handler)(
        sig, reinterpret_cast<siginfo_t *>(info), uctx);
    return pc;
  }

  const auto handler = current.handler;
  const uptr value = reinterpret_cast<uptr>(handler);
  if (value == sig_dfl || value == sig_ign)
    return value;
  const uptr pc = STRIP_PAC_PC(handler);
  handler(sig);
  return pc;
}

void CopyDarwinSignalInfo(void *destination, const void *source) {
  __sanitizer::internal_memcpy(destination, source, sizeof(siginfo_t));
}

void CopyDarwinSignalContext(DarwinSignalContext *destination,
                             const void *source) {
  const ucontext_t *original = reinterpret_cast<const ucontext_t *>(source);
  __sanitizer::internal_memcpy(&destination->context, original,
                               sizeof(destination->context));
  if (!original->uc_mcontext)
    return;

  // uc_mcsize, not sizeof(mcontext_t), describes the supplied machine state.
  // This also preserves extended x86_64 register contexts when available.
  CHECK_LE(original->uc_mcsize, sizeof(destination->mcontext));
  __sanitizer::internal_memset(&destination->mcontext, 0,
                               sizeof(destination->mcontext));
  __sanitizer::internal_memcpy(&destination->mcontext, original->uc_mcontext,
                               original->uc_mcsize);
  destination->context.uc_mcontext =
      reinterpret_cast<decltype(destination->context.uc_mcontext)>(
          &destination->mcontext);
}

}  // namespace __tsan

#endif  // SANITIZER_APPLE
