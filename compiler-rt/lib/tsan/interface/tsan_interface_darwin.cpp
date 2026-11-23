//===-- tsan_interface_darwin.cpp -----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Darwin-only TSan callbacks. These hooks connect the shared sanitizer
// runtime and Darwin fork testing to TSan without putting platform-specific
// interface definitions in the core runtime.
//
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE

#include "sanitizer_common/sanitizer_internal_defs.h"
#include "../rtl/tsan_rtl.h"

// Test hook called before fork while TSan has its internal locks held.
// Keep it weak so Darwin tests can replace the default no-op.
SANITIZER_WEAK_DEFAULT_IMPL
void __tsan_test_only_on_fork() {}

#if !SANITIZER_GO
// Override sanitizer_common's weak fallback. The internal write path must
// bypass TSan's own interceptors to prevent recursion during diagnostics.
extern void __tsan_set_in_internal_write_call(bool value) {
  __tsan::cur_thread_init()->in_internal_write_call = value;
}
#endif  // !SANITIZER_GO

#endif  // SANITIZER_APPLE
