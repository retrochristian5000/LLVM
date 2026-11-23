//===-- tsan_pragma.h ------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Thread-local ignore scopes used by runtime annotations and interceptors.
// Clang parses #pragma directives at compile time; this is not a pragma parser.

#ifndef TSAN_PRAGMA_H
#define TSAN_PRAGMA_H

#include "tsan_defs.h"

namespace __tsan {

void ThreadIgnoreBegin(ThreadState *thr, uptr pc);
void ThreadIgnoreEnd(ThreadState *thr);
void ThreadIgnoreSyncBegin(ThreadState *thr, uptr pc);
void ThreadIgnoreSyncEnd(ThreadState *thr);

}  // namespace __tsan

#endif  // TSAN_PRAGMA_H
