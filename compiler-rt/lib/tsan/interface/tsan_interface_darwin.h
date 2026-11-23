//===-- tsan_interface_darwin.h ---------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Darwin-specific TSan interface declarations.
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_INTERFACE_DARWIN_H
#define TSAN_INTERFACE_DARWIN_H

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE
// Weakly defined by tsan_interface_darwin.cpp. Tests may override this hook.
extern "C" void __tsan_test_only_on_fork();
#endif

#endif  // TSAN_INTERFACE_DARWIN_H
