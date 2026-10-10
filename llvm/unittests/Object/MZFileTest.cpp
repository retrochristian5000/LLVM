//===- MZFileTest.cpp - DOS MZ reader regression tests --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/BinaryFormat/MZ.h"
#include "llvm/BinaryFormat/Magic.h"
#include "llvm/Object/Binary.h"
#include "llvm/Object/MZFile.h"
#include "llvm/Support/Error.h"
#include "gtest/gtest.h"
#include <array>
#include <cstring>

using namespace llvm;
using namespace llvm::object;

namespace {
static std::array<char, 128> makeMZImage() {
  std::array<char, 128> Data{};
  MZ::Header Hdr{};
  Hdr.Magic[0] = 'M';
  Hdr.Magic[1] = 'Z';
  Hdr.PagesInFile = 1;
  Hdr.BytesInLastPage = 96;
  Hdr.HeaderParagraphs = 2;
  Hdr.RelocationCount = 1;
  Hdr.RelocationTableOffset = sizeof(MZ::Header);
  Hdr.InitialSP = 0xfffe;
  std::memcpy(Data.data(), &Hdr, sizeof(Hdr));
  MZ::Relocation R{};
  R.Offset = 4;
  std::memcpy(Data.data() + sizeof(Hdr), &R, sizeof(R));
  Data[32] = char(0xcd);
  Data[33] = 0x20;
  return Data;
}

TEST(MZFileTest, LoadsImageRelocationsAndOverlay) {
  auto Data = makeMZImage();
  MemoryBufferRef Buffer(StringRef(Data.data(), Data.size()), "dos.exe");
  EXPECT_EQ(file_magic::dos_executable, identify_magic(Buffer.getBuffer()));
  auto Binary = cantFail(createBinary(Buffer));
  ASSERT_TRUE(Binary->isMZ());
  const auto *Obj = static_cast<const MZFile *>(Binary.get());
  EXPECT_EQ(32u, Obj->getHeaderSize());
  EXPECT_EQ(96u, Obj->getDeclaredFileSize());
  EXPECT_EQ(64u, Obj->getLoadImage().size());
  EXPECT_EQ(char(0xcd), Obj->getLoadImage()[0]);
  EXPECT_EQ(32u, Obj->getOverlay().size());
  ASSERT_EQ(1u, Obj->relocations().size());
  EXPECT_EQ(4u, uint16_t(Obj->relocations()[0].Offset));
}

TEST(MZFileTest, RejectsTruncationAndRelocationOverflow) {
  auto Data = makeMZImage();
  {
    MemoryBufferRef Buffer(StringRef(Data.data(), 80), "short.exe");
    auto Obj = createBinary(Buffer);
    EXPECT_FALSE(Obj);
    consumeError(Obj.takeError());
  }
  {
    MZ::Header Hdr;
    std::memcpy(&Hdr, Data.data(), sizeof(Hdr));
    Hdr.RelocationCount = 2; // One relocation fits in the 32-byte header.
    std::memcpy(Data.data(), &Hdr, sizeof(Hdr));
    MemoryBufferRef Buffer(StringRef(Data.data(), Data.size()), "bad.exe");
    auto Obj = createBinary(Buffer);
    EXPECT_FALSE(Obj);
    consumeError(Obj.takeError());
  }
}

TEST(MZFileTest, ExtendedFormatsAreNotPlainMZ) {
  auto Data = makeMZImage();
  Data[0x3c] = 0x40;
  Data[0x40] = 'N'; Data[0x41] = 'E';
  EXPECT_EQ(file_magic::windows_ne,
            identify_magic(StringRef(Data.data(), Data.size())));
  Data[0x40] = 'P'; Data[0x41] = 'E';
  Data[0x42] = 0; Data[0x43] = 0;
  EXPECT_EQ(file_magic::pecoff_executable,
            identify_magic(StringRef(Data.data(), Data.size())));
}
} // namespace
