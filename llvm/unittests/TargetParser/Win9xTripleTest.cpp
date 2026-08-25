//===----------- Win9xTripleTest.cpp - Win9x triple tests ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/TargetParser/Triple.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

TEST(Win9xTripleTest, ParsesAsLegacyWin32COFF) {
  Triple T("i386-pc-win9x");

  EXPECT_EQ(Triple::x86, T.getArch());
  EXPECT_EQ(Triple::PC, T.getVendor());
  EXPECT_EQ(Triple::Win32, T.getOS());
  EXPECT_TRUE(T.isOSWindows());
  EXPECT_EQ(Triple::COFF, T.getObjectFormat());
}

} // namespace
