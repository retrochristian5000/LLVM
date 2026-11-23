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

#include <signal.h>
#include <sys/ucontext.h>

#include "sanitizer_common/sanitizer_libc.h"

namespace __tsan {

static_assert(sizeof(ucontext_t) == sizeof(DarwinSignalContext),
              "Darwin ucontext_t has changed; update the deferred context "
              "storage before compiling TSan");
static_assert(sizeof(siginfo_t) <= 128,
              "Darwin siginfo_t exceeds POSIX interceptor storage");

void CopyDarwinSignalInfo(void *destination, const void *source) {
  __sanitizer::internal_memcpy(destination, source, sizeof(siginfo_t));
}

void CopyDarwinSignalContext(DarwinSignalContext *destination,
                             const void *source) {
  __sanitizer::internal_memcpy(destination, source, sizeof(ucontext_t));
}

}  // namespace __tsan

#endif  // SANITIZER_APPLE
