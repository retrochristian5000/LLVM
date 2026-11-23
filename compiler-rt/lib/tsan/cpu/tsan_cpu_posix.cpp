//===-- tsan_cpu_posix.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Validate CPU/ABI virtual-address widths supported by TSan's POSIX
// Linux/BSD memory layouts. Hardware probing stays in tsan_cpu.cpp;
// Darwin uses fixed mappings and its own CPU/ABI support.
//
//===----------------------------------------------------------------------===//

#include "tsan_cpu_posix.h"

#if SANITIZER_LINUX || SANITIZER_FREEBSD || SANITIZER_NETBSD

#include "sanitizer_common/sanitizer_common.h"
#include "../rtl/tsan_platform.h"

namespace __tsan {

void ValidatePosixCpuVmaSize() {
#if defined(__aarch64__)
# if !SANITIZER_GO
  if (vmaSize != 39 && vmaSize != 42 && vmaSize != 47 && vmaSize != 48) {
    Printf("FATAL: ThreadSanitizer: unsupported VMA range\n");
    Printf("FATAL: Found %zd - Supported 39, 42, 47 and 48\n", vmaSize);
    Die();
  }
#    else
  if (vmaSize != 48) {
    Printf("FATAL: ThreadSanitizer: unsupported VMA range\n");
    Printf("FATAL: Found %zd - Supported 48\n", vmaSize);
    Die();
  }
#    endif
#  elif SANITIZER_LOONGARCH64
#    if !SANITIZER_GO
  if (vmaSize != 47) {
    Printf("FATAL: ThreadSanitizer: unsupported VMA range\n");
    Printf("FATAL: Found %zd - Supported 47\n", vmaSize);
    Die();
  }
#    else
  if (vmaSize != 47) {
    Printf("FATAL: ThreadSanitizer: unsupported VMA range\n");
    Printf("FATAL: Found %zd - Supported 47\n", vmaSize);
    Die();
  }
#    endif
#  elif defined(__powerpc64__)
#    if !SANITIZER_GO
  if (vmaSize != 44 && vmaSize != 46 && vmaSize != 47) {
    Printf("FATAL: ThreadSanitizer: unsupported VMA range\n");
    Printf("FATAL: Found %zd - Supported 44, 46, and 47\n", vmaSize);
    Die();
  }
#    else
  if (vmaSize != 46 && vmaSize != 47) {
    Printf("FATAL: ThreadSanitizer: unsupported VMA range\n");
    Printf("FATAL: Found %zd - Supported 46, and 47\n", vmaSize);
    Die();
  }
#    endif
#  elif defined(__mips64)
#    if !SANITIZER_GO
  if (vmaSize != 40) {
    Printf("FATAL: ThreadSanitizer: unsupported VMA range\n");
    Printf("FATAL: Found %zd - Supported 40\n", vmaSize);
    Die();
  }
#    else
  if (vmaSize != 47) {
    Printf("FATAL: ThreadSanitizer: unsupported VMA range\n");
    Printf("FATAL: Found %zd - Supported 47\n", vmaSize);
    Die();
  }
#    endif
#  elif SANITIZER_RISCV64
#    if !SANITIZER_GO
  if (vmaSize != 39 && vmaSize != 48) {
    Printf("FATAL: ThreadSanitizer: unsupported VMA range\n");
    Printf("FATAL: Found %zd - Supported 39 and 48\n", vmaSize);
    Die();
  }
#    else
  if (vmaSize != 39 && vmaSize != 48) {
    Printf("FATAL: ThreadSanitizer: unsupported VMA range\n");
    Printf("FATAL: Found %zd - Supported 39 and 48\n", vmaSize);
    Die();
  }
#    endif
#  endif
}

}  // namespace __tsan

#endif  // SANITIZER_LINUX || SANITIZER_FREEBSD || SANITIZER_NETBSD
