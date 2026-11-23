//===-- tsan_interface_aarch64_test.cpp -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tsan_interface_aarch64.h"

#if defined(__aarch64__) && !SANITIZER_GO

#include "sanitizer_common/sanitizer_ptrauth.h"
#include "tsan_defs.h"
#include "gtest/gtest.h"

namespace __tsan {

TEST(InterfaceAArch64, NormalizesReturnPc) {
  void *const pc = __builtin_return_address(0);
  const uptr expected = STRIP_PAC_PC(pc);
  const uptr actual =
      reinterpret_cast<uptr>(__tsan_interface_aarch64_strip_pc(pc));
  EXPECT_EQ(expected, actual);
}

TEST(InterfaceAArch64, PreservesNullPc) {
  EXPECT_EQ(nullptr, __tsan_interface_aarch64_strip_pc(nullptr));
}

}  // namespace __tsan

#endif  // __aarch64__ && !SANITIZER_GO
