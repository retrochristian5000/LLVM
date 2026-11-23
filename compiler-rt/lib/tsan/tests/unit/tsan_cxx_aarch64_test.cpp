//===-- tsan_cxx_aarch64_test.cpp ----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "../../cxx/tsan_cxx_aarch64.h"

#if defined(__aarch64__) && !SANITIZER_GO
#include "sanitizer_common/sanitizer_ptrauth.h"
#include "gtest/gtest.h"

namespace __tsan {

TEST(CxxAArch64, VptrUpdateProbe) {
  int first = 0;
  int second = 0;
  void *slot = &first;
  void *caller_pc = __builtin_return_address(0);

  const CxxAarch64VptrProbe unchanged =
      __tsan_cxx_aarch64_vptr_probe(&slot, &first, caller_pc);
  EXPECT_EQ(0u, unchanged.changed);

  const CxxAarch64VptrProbe changed =
      __tsan_cxx_aarch64_vptr_probe(&slot, &second, caller_pc);
  EXPECT_EQ(1u, changed.changed);
  EXPECT_EQ(STRIP_PAC_PC(caller_pc), changed.caller_pc);
  // The probe is observational: only the instrumented C++ code may write.
  EXPECT_EQ(&first, slot);
}

TEST(CxxAArch64, NullVptrIsComparedWithoutDereferencingValue) {
  void *slot = nullptr;
  const CxxAarch64VptrProbe same =
      __tsan_cxx_aarch64_vptr_probe(&slot, nullptr, nullptr);
  EXPECT_EQ(0u, same.changed);

  int replacement = 0;
  const CxxAarch64VptrProbe different =
      __tsan_cxx_aarch64_vptr_probe(&slot, &replacement, nullptr);
  EXPECT_EQ(1u, different.changed);
  EXPECT_EQ(0u, different.caller_pc);
}

}  // namespace __tsan
#endif  // defined(__aarch64__) && !SANITIZER_GO
