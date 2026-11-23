//===-- tsan_cxx_aarch64.S -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// AArch64 ABI helper for the C++ virtual-pointer update callback.
// Compare the current vptr slot with the proposed value and return the
// comparison plus the caller PC as a two-register result (x0, x1).
//
// x0 = address of a vptr slot, x1 = proposed vptr, x2 = caller PC.
// On return: x0 = caller PC (PAC stripped if a change was observed),
//            x1 = 1 if the stored vptr differs, 0 otherwise.
// The no-change path does not normalize x0; callers must ignore that PC.
// The helper never writes to the vptr slot or performs TSan accounting.
//
//===----------------------------------------------------------------------===//

#if defined(__aarch64__)
.text
.p2align 2

#if defined(__APPLE__)
.globl ___tsan_cxx_aarch64_vptr_probe
.private_extern ___tsan_cxx_aarch64_vptr_probe
___tsan_cxx_aarch64_vptr_probe:
#else
.globl __tsan_cxx_aarch64_vptr_probe
.hidden __tsan_cxx_aarch64_vptr_probe
.type __tsan_cxx_aarch64_vptr_probe, %function
__tsan_cxx_aarch64_vptr_probe:
#endif
  bti c

  ldr x8, [x0]
  cmp x8, x1
  cset x1, ne
  mov x0, x2
  cbz x1, 1f

#if defined(__arm64e__) || defined(__ARM_FEATURE_PAC_DEFAULT)
  // XPACLRI acts on LR (x30). Preserve the function's real return address.
  mov x9, x30
  mov x30, x0
  hint #7
  mov x0, x30
  mov x30, x9
#endif

1:
  ret

#if !defined(__APPLE__)
.size __tsan_cxx_aarch64_vptr_probe, .-__tsan_cxx_aarch64_vptr_probe
.section .note.GNU-stack,"",%progbits
#endif
#endif  // defined(__aarch64__)
