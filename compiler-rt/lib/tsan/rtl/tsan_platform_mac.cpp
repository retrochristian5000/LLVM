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
#include "../interface/tsan_interface_got.h"
#include "tsan_macho.h"
#include "tsan_platform.h"
#include "tsan_rtl.h"
#include "../flags/tsan_flags.h"
#include "../warn/tsan_warn_darwin.h"

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
    ReportDarwinVirtualMemoryLimit(max_vm, HiAppMemEnd());
    Die();
  }
  // In some configurations, the max_vm is expanded, but much of this space is
  // already mapped. TSAN will not work in this configuration.
  if (!MemoryRangeIsAvailable(HiAppMemEnd() - 1, HiAppMemEnd() - 1)) {
    ReportDarwinOccupiedShadowAddress(HiAppMemEnd() - 1);
    Die();
  }
#endif
}

void InitializePlatform() {
  DisableCoreDumperIfNecessary();
  VPrintf(2, "ThreadSanitizer: main Mach-O executable architecture: %s\n",
          MachOExecutableArchitecture());
  if (Verbosity() >= 2) {
    MachOGOTSlots slots;
    if (GetMainExecutableMachOGOTSlots(&slots))
      VPrintf(2,
              "ThreadSanitizer: main Mach-O symbol-pointer slots: "
              "lazy=%zu, non-lazy=%zu\n",
              slots.lazy, slots.non_lazy);
#if defined(__aarch64__) && !SANITIZER_GO
    VPrintf(2, "ThreadSanitizer: AArch64 Mach-O GOT data relocation: %s\n",
            VerifyAArch64GOTAnchor() ? "resolved" : "mismatch");
#endif
  }
#if !SANITIZER_GO
  if (!CheckAndProtect(true, true, true)) {
    ReportDarwinIncompatibleMemoryLayout();
    Die();
  }

  InitializeDarwinThreadSupport();
#endif

  InitializeDarwinLongJmp();
}


}  // namespace __tsan

#endif  // SANITIZER_APPLE
