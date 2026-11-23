//===-- tsan_platform_mac.cpp ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
// Mac-specific code.
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"
#if SANITIZER_APPLE

#include "sanitizer_common/sanitizer_atomic.h"
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_libc.h"
#include "sanitizer_common/sanitizer_posix.h"
#include "sanitizer_common/sanitizer_procmaps.h"
#include "tsan_platform.h"
#include "tsan_rtl.h"
#include "tsan_flags.h"

#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>

namespace __tsan {

#  if !SANITIZER_GO
void InitializeShadowMemoryPlatform() { }

#endif

void InitializePlatformEarly() {
#  if !SANITIZER_GO && SANITIZER_IOS
  uptr max_vm = GetMaxUserVirtualAddress() + 1;
  if (max_vm < HiAppMemEnd()) {
    Report(
        "ThreadSanitizer: Unsupported virtual memory layout:\n\tVM address "
        "limit = %p\n\tExpected %p.\n",
        (void*)max_vm, (void*)HiAppMemEnd());
    Die();
  }
  // In some configurations, the max_vm is expanded, but much of this space is
  // already mapped. TSAN will not work in this configuration.
  if (!MemoryRangeIsAvailable(HiAppMemEnd() - 1, HiAppMemEnd() - 1)) {
    Report(
        "ThreadSanitizer: Unsupported virtual memory layout: Address %p is "
        "already mapped.\n",
        (void*)(HiAppMemEnd() - 1));
    Die();
  }
#endif
}

void InitializePlatform() {
  DisableCoreDumperIfNecessary();
#if !SANITIZER_GO
  if (!CheckAndProtect(true, true, true)) {
    Printf("FATAL: ThreadSanitizer: found incompatible memory layout.\n");
    Die();
  }

  InitializeDarwinThreadSupport();
#endif

  InitializeDarwinLongJmp();
}


}  // namespace __tsan

#endif  // SANITIZER_APPLE
