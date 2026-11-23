//===-- tsan_interface_aarch64.h -------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_INTERFACE_AARCH64_H
#define TSAN_INTERFACE_AARCH64_H

// Private ABI support for AArch64 TSan. Unlike the public __tsan_* API,
// this symbol must not become a stable external instrumentation interface.
#if defined(__aarch64__) && !SANITIZER_GO
extern "C" __attribute__((visibility("hidden"))) void *
__tsan_interface_aarch64_strip_pc(void *pc);
#endif

#endif  // TSAN_INTERFACE_AARCH64_H
