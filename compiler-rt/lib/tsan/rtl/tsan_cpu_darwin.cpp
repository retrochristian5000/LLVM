//===-- tsan_cpu_darwin.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Darwin libc jump-buffer CPU/ABI support for x86_64, AArch64, and ARM64e.
// Keep Mach VM accounting in tsan_platform_darwin.cpp. Darwin TSan uses
// fixed shadow-memory mappings, unlike Linux's runtime VMA-width selection.

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_mac.h"
#include "sanitizer_common/sanitizer_ptrauth.h"
#include "tsan_platform.h"

#include <pthread.h>

namespace __tsan {

static uptr longjmp_xor_key = 0;

void InitializeDarwinLongJmp() {
  if (GetMacosAlignedVersion() >= MacosVersion(10, 14)) {
    // Libsystem currently uses a process-global key; this might change.
    const unsigned kTLSLongjmpXorKeySlot = 0x7;
    longjmp_xor_key = (uptr)pthread_getspecific(kTLSLongjmpXorKeySlot);
  }
}

static uptr DarwinLongJmpStackSlot() {
#if defined(__aarch64__)
  // The Darwin ARM64 jump-buffer stack slot changed with the libc ABI.
  return GetMacosAlignedVersion() >= MacosVersion(10, 14) ? 12 : 13;
#else
  // Darwin x86_64 retains a distinct jump-buffer layout.
  return 2;
#endif
}

uptr ExtractLongJmpSp(uptr *env) {
  uptr mangled_sp = env[DarwinLongJmpStackSlot()];
  uptr sp = mangled_sp ^ longjmp_xor_key;
  sp = (uptr)ptrauth_auth_data((void *)sp, ptrauth_key_asdb,
                               ptrauth_string_discriminator("sp"));
  return sp;
}

}  // namespace __tsan

#endif  // SANITIZER_APPLE
