//===-- tsan_cpu_posix.h -------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// POSIX Linux/BSD CPU virtual-address-width validation.
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_CPU_POSIX_H
#define TSAN_CPU_POSIX_H

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_LINUX || SANITIZER_FREEBSD || SANITIZER_NETBSD
namespace __tsan {

void ValidatePosixCpuVmaSize();

}  // namespace __tsan
#endif

#endif  // TSAN_CPU_POSIX_H
