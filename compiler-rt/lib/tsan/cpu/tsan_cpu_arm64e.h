//===-- tsan_cpu_arm64e.h -------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Private Darwin arm64e ABI entry point for authenticating a saved libc SP.
// C++ supplies the discriminator; assembly applies the Data-B key and traps
// on authentication failure. Not part of the TSan instrumentation API.
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_CPU_ARM64E_H
#define TSAN_CPU_ARM64E_H

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE && defined(__arm64e__) && !SANITIZER_GO
#include "sanitizer_common/sanitizer_internal_defs.h"

extern "C" __attribute__((visibility("hidden"))) __sanitizer::uptr
__tsan_cpu_arm64e_auth_jmp_sp(__sanitizer::uptr signed_sp,
                              __sanitizer::uptr discriminator);
#endif

#endif  // TSAN_CPU_ARM64E_H
