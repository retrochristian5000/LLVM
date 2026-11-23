//===-- tsan_cxx_aarch64.h ------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Internal AArch64 virtual-pointer instrumentation ABI. This is not part of
// the public TSan interface. The two-uptr return value uses x0/x1 on AArch64.
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_CXX_AARCH64_H
#define TSAN_CXX_AARCH64_H

#include "sanitizer_common/sanitizer_platform.h"

#if defined(__aarch64__) && !SANITIZER_GO
#include "sanitizer_common/sanitizer_internal_defs.h"

namespace __tsan {
struct CxxAarch64VptrProbe {
  uptr caller_pc;  // Valid only if changed is nonzero.
  uptr changed;
};
static_assert(sizeof(CxxAarch64VptrProbe) == 2 * sizeof(uptr),
              "AArch64 vptr probe must return in x0 and x1");
}  // namespace __tsan

extern "C" __attribute__((visibility("hidden"))) __tsan::CxxAarch64VptrProbe
__tsan_cxx_aarch64_vptr_probe(void *const *vptr_slot, void *new_val,
                               void *caller_pc);
#endif

#endif  // TSAN_CXX_AARCH64_H
