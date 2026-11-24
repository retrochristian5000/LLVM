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

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_libc.h"

namespace __tsan {

static_assert(sizeof(siginfo_t) <= 128,
              "Darwin siginfo_t exceeds POSIX interceptor storage");

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
