//===- DOSDeviceTest.cpp - Raw DOS device-header regression tests --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Object/DOSDevice.h"
#include "llvm/Support/Error.h"
#include "gtest/gtest.h"
#include <array>

using namespace llvm;
using namespace llvm::object;

namespace {
static std::array<char, 64> makeDriver() {
  std::array<char, 64> B{};
  B[0] = B[1] = B[2] = B[3] = char(0xff);
  B[4] = 0; B[5] = char(0x80); // Character device.
  B[6] = 18; B[8] = 19;
  const char Name[8] = {'W', 'H', 'P', 'T', 'E', 'S', 'T', ' '};
  for (unsigned I = 0; I < 8; ++I)
    B[10 + I] = Name[I];
  B[18] = B[19] = char(0xcb); // RETF
  return B;
}

TEST(DOSDeviceTest, ReadsCharacterDeviceHeader) {
  auto Data = makeDriver();
  auto H = cantFail(DOSDevice::parseHeader(
      MemoryBufferRef(StringRef(Data.data(), Data.size()), "char.sys")));
  EXPECT_EQ(0xffffffffu, H.NextDevice);
  EXPECT_TRUE(H.isCharacterDevice());
  EXPECT_EQ(0x8000, H.Attributes);
  EXPECT_EQ(18, H.StrategyOffset);
  EXPECT_EQ(19, H.InterruptOffset);
  EXPECT_EQ("WHPTEST ", StringRef(H.NameOrUnits.data(), 8));
}

TEST(DOSDeviceTest, ReadsBlockDeviceAndChainedHeaderPointer) {
  auto Data = makeDriver();
  Data[5] = 0;  // Block device.
  Data[10] = 2; // Initial number of units, other bytes are reserved.
  Data[0] = 32; Data[1] = Data[2] = Data[3] = 0;
  auto H = cantFail(DOSDevice::parseHeader(
      MemoryBufferRef(StringRef(Data.data(), Data.size()), "block.sys")));
  EXPECT_FALSE(H.isCharacterDevice());
  EXPECT_EQ(2, static_cast<unsigned char>(H.NameOrUnits[0]));
  EXPECT_EQ(32u, H.NextDevice);
}

TEST(DOSDeviceTest, RejectsTruncatedAndInvalidHeaders) {
  auto Data = makeDriver();
  {
    auto H = DOSDevice::parseHeader(
        MemoryBufferRef(StringRef(Data.data(), 17), "short.sys"));
    EXPECT_FALSE(H);
    consumeError(H.takeError());
  }
  {
    Data[0] = 1; // Not a valid on-disk first-header link.
    auto H = DOSDevice::parseHeader(
        MemoryBufferRef(StringRef(Data.data(), Data.size()), "link.sys"));
    EXPECT_FALSE(H);
    consumeError(H.takeError());
    Data[0] = char(0xff);
  }
  {
    Data[6] = 64; // Strategy entry point at end of file.
    auto H = DOSDevice::parseHeader(
        MemoryBufferRef(StringRef(Data.data(), Data.size()), "entry.sys"));
    EXPECT_FALSE(H);
    consumeError(H.takeError());
  }
}
} // namespace
