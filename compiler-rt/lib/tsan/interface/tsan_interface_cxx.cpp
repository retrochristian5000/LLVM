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

#if !SANITIZER_GO

using namespace __tsan;

#define CALLERPC ((uptr)__builtin_return_address(0))

void __tsan_vptr_update(void **vptr_p, void *new_val) {
  if (*vptr_p == new_val)
    return;
  MemoryAccess(cur_thread(), CALLERPC, (uptr)vptr_p, sizeof(*vptr_p),
               kAccessWrite | kAccessVptr);
}

void __tsan_vptr_read(void **vptr_p) {
  MemoryAccess(cur_thread(), CALLERPC, (uptr)vptr_p, sizeof(*vptr_p),
               kAccessRead | kAccessVptr);
}

#undef CALLERPC

#endif  // !SANITIZER_GO
