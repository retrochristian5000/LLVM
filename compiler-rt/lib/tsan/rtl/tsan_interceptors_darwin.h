//===-- tsan_interceptors_darwin.h ----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_INTERCEPTORS_DARWIN_H
#define TSAN_INTERCEPTORS_DARWIN_H

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE

#include <sys/ucontext.h>

#include "sanitizer_common/sanitizer_internal_defs.h"
#include "sanitizer_common/sanitizer_platform_limits_posix.h"

namespace __tsan {

// Darwin's signal ucontext points at a separate machine context. A deferred
// callback must not retain the original pointer into the signal stack.
// Reserve storage for the widest machine context supported by this SDK.
struct DarwinSignalContext {
  ucontext_t context;
#if defined(__x86_64__) && defined(_STRUCT_MCONTEXT_AVX512_64_FULL)
  _STRUCT_MCONTEXT_AVX512_64_FULL mcontext;
#elif defined(__x86_64__) && defined(_STRUCT_MCONTEXT_AVX512_64)
  _STRUCT_MCONTEXT_AVX512_64 mcontext;
#else
  _STRUCT_MCONTEXT mcontext;
#endif
};

// Invoke the registered callback with its native signature. Return a
// diagnostic PC, not a callable pointer reconstructed from an integer.
uptr InvokeDarwinSignalHandler(const __sanitizer_sigaction &action, int sig,
                               __sanitizer_siginfo *info, void *uctx);

void CopyDarwinSignalInfo(void *destination, const void *source);
void CopyDarwinSignalContext(DarwinSignalContext *destination,
                             const void *source);

}  // namespace __tsan

#endif  // SANITIZER_APPLE
#endif  // TSAN_INTERCEPTORS_DARWIN_H
