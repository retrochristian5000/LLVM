//===-- tsan_ignore.h ------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Thread-local ignore scopes used by runtime annotations and interceptors.
// Clang parses compiler pragmas; TSan manages only runtime ignore state.

#ifndef TSAN_IGNORE_H
#define TSAN_IGNORE_H

#include "tsan_defs.h"

namespace __tsan {

#if defined(__aarch64__) && !SANITIZER_GO
// Checked leaf assembly helper; never accesses ThreadState layout.
extern "C" int __tsan_ignore_aarch64_depth_step(int *depth, int delta);
#endif

void ThreadIgnoreBegin(ThreadState *thr, uptr pc);
void ThreadIgnoreEnd(ThreadState *thr);
void ThreadIgnoreSyncBegin(ThreadState *thr, uptr pc);
void ThreadIgnoreSyncEnd(ThreadState *thr);

}  // namespace __tsan

#endif  // TSAN_IGNORE_H
