//===-- tsan_interface_cxx.cpp --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// C++ virtual-table instrumentation entry points. Keep this ABI layer
// separate from the generic memory access interface and Java integration.
// C++ new/delete interception remains in rtl/tsan_new_delete.cpp.
//
//===----------------------------------------------------------------------===//

#include "tsan_interface.h"
#include "../rtl/tsan_rtl.h"
#include "../cxx/tsan_cxx_aarch64.h"
#include "tsan_interface_aarch64.h"

#if !SANITIZER_GO

using namespace __tsan;

#define CALLERPC ((uptr)__builtin_return_address(0))

void __tsan_vptr_update(void **vptr_p, void *new_val) {
#if defined(__aarch64__)
  // AArch64 returns the comparison and normalized caller PC in x0/x1.
  // The slot is only read; MemoryAccess still reports the virtual write.
  const CxxAarch64VptrProbe probe = __tsan_cxx_aarch64_vptr_probe(
      vptr_p, new_val, __builtin_return_address(0));
  if (!probe.changed)
    return;
  const uptr pc = probe.caller_pc;
#else
  if (*vptr_p == new_val)
    return;
  const uptr pc = CALLERPC;
#endif
  MemoryAccess(cur_thread(), pc, (uptr)vptr_p, sizeof(*vptr_p),
               kAccessWrite | kAccessVptr);
}

void __tsan_vptr_read(void **vptr_p) {
#if defined(__aarch64__)
  // Share the existing AArch64 caller-PC normalization; do not duplicate it.
  const uptr pc = reinterpret_cast<uptr>(
      __tsan_interface_aarch64_strip_pc(__builtin_return_address(0)));
#else
  const uptr pc = CALLERPC;
#endif
  MemoryAccess(cur_thread(), pc, (uptr)vptr_p, sizeof(*vptr_p),
               kAccessRead | kAccessVptr);
}

#undef CALLERPC

#endif  // !SANITIZER_GO
