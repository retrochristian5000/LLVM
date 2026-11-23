//===-- tsan_stack_aarch64.h ----------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Internal AArch64 stack ABI helper, shared by setjmp recording and
// Darwin jump-buffer decoding. Not part of the public TSan interface.
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_STACK_AARCH64_H
#define TSAN_STACK_AARCH64_H

#include "sanitizer_common/sanitizer_platform.h"

#if defined(__aarch64__) && !SANITIZER_GO
#include "sanitizer_common/sanitizer_internal_defs.h"

// Returns nonzero for a valid non-null 16-byte-aligned AArch64 call-site SP.
extern "C" __attribute__((visibility("hidden"))) int
__tsan_stack_aarch64_valid_sp(__sanitizer::uptr sp);
#endif

#endif  // TSAN_STACK_AARCH64_H
