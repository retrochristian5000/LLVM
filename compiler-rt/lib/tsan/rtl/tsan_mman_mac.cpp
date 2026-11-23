//===-- tsan_mman_mac.cpp -------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Apple malloc-zone allocation and free hook dispatch. The zone interceptors
// live in tsan_malloc_mac.cpp; Darwin's signal-unsafe allocation reporting
// remains in tsan_mman_darwin.cpp. Non-Apple hooks stay in tsan_mman.cpp.
//
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE

#include "tsan_mman.h"
#include "tsan_rtl.h"

namespace __tsan {

void invoke_malloc_hook(void *ptr, uptr size) {
  ThreadState *thr = cur_thread();
  if (ctx == 0 || !ctx->initialized || thr->ignore_interceptors)
    return;
  RunMallocHooks(ptr, size);
}

void invoke_free_hook(void *ptr) {
  ThreadState *thr = cur_thread();
  if (ctx == 0 || !ctx->initialized || thr->ignore_interceptors)
    return;
  RunFreeHooks(ptr);
}

}  // namespace __tsan

#endif  // SANITIZER_APPLE
