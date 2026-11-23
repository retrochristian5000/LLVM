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

#include "sanitizer_common/sanitizer_internal_defs.h"

namespace __tsan {

// The signal handler receives a ucontext_t header, not the separately
// allocated machine context to which uc_mcontext points. The Darwin
// implementation checks this buffer size against the active SDK.
struct DarwinSignalContext {
#if SANITIZER_WORDSIZE == 64
  __sanitizer::u64 opaque[56 / sizeof(__sanitizer::u64)];
#else
  __sanitizer::u32 opaque[32 / sizeof(__sanitizer::u32)];
#endif
};

void CopyDarwinSignalInfo(void *destination, const void *source);
void CopyDarwinSignalContext(DarwinSignalContext *destination,
                             const void *source);

}  // namespace __tsan

#endif  // SANITIZER_APPLE
#endif  // TSAN_INTERCEPTORS_DARWIN_H
