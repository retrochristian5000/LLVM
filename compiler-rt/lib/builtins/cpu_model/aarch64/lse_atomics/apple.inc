// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <errno.h>
#include <sys/sysctl.h>

// XNU exposes the same capability through modern and legacy sysctl names.
// A successful negative answer is authoritative. Only try the legacy name
// when the modern name is absent; malformed or denied queries fail closed.
static _Bool darwin_has_lse_atomics(void) {
  int enabled = 0;
  size_t size = sizeof(enabled);
  if (sysctlbyname("hw.optional.arm.FEAT_LSE", &enabled, &size, 0, 0) == 0)
    return size == sizeof(enabled) && enabled == 1;
  if (errno != ENOENT)
    return false;

  enabled = 0;
  size = sizeof(enabled);
  return sysctlbyname("hw.optional.armv8_1_atomics", &enabled, &size, 0, 0) == 0 &&
         size == sizeof(enabled) && enabled == 1;
}

static void CONSTRUCTOR_ATTRIBUTE init_have_lse_atomics(void) {
  // Until this constructor runs, the zero-initialized flag safely selects
  // the exclusive-instruction implementation, including during bootstrap.
  const int saved_errno = errno;
  __atomic_store_n(&__aarch64_have_lse_atomics, darwin_has_lse_atomics(),
                   __ATOMIC_RELAXED);
  errno = saved_errno;
}
