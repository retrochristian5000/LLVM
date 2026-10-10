//===- NEFileTest.cpp - Tests for Windows NE binaries -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/BinaryFormat/NE.h"
#include "llvm/Object/NEFile.h"
#include "llvm/Support/Error.h"
#include "gtest/gtest.h"
#include <array>
#include <cstring>

using namespace llvm;
using namespace llvm::object;

namespace {

static std::array<char, 0x88> makeNEImage() {
  std::array<char, 0x88> Data{};
  Data[0] = 'M';
  Data[1] = 'Z';
  Data[0x3c] = 0x40;

  NE::Header Hdr{};
  Hdr.Signature = 0x454e;
  Hdr.LinkerVersion = 5;
  Hdr.LinkerRevision = 1;
  Hdr.SegmentTableOffset = sizeof(NE::Header);
  Hdr.SegmentCount = 1;
  Hdr.SegmentAlignmentShift = 4;
  Hdr.TargetOS = 2;
  std::memcpy(Data.data() + 0x40, &Hdr, sizeof(Hdr));

  NE::Segment Seg{};
  Seg.DataOffset = 0x20;
  Seg.DataLength = 0x1234;
  Seg.Flags = NE::SegmentData | NE::SegmentRelocations;
  Seg.MinimumAllocation = 0x2000;
  std::memcpy(Data.data() + 0x40 + sizeof(NE::Header), &Seg, sizeof(Seg));
  return Data;
}

TEST(NEFileTest, ParsesHeaderAndSegments) {
  auto Data = makeNEImage();
  MemoryBufferRef Buffer(StringRef(Data.data(), Data.size()), "test.ne");
  std::unique_ptr<NEFile> Obj = cantFail(NEFile::create(Buffer));

  EXPECT_EQ(0x40u, Obj->getHeaderOffset());
  EXPECT_EQ(0x454eu, uint16_t(Obj->getHeader().Signature));
  EXPECT_EQ(5u, Obj->getHeader().LinkerVersion);
  EXPECT_EQ(1u, Obj->getHeader().LinkerRevision);
  EXPECT_EQ(4u, Obj->getSegmentAlignmentShift());

  ASSERT_EQ(1u, Obj->segments().size());
  const NE::Segment &Seg = Obj->segments().front();
  EXPECT_EQ(0x20u, uint16_t(Seg.DataOffset));
  EXPECT_EQ(0x1234u, uint16_t(Seg.DataLength));
  EXPECT_EQ(uint16_t(NE::SegmentData | NE::SegmentRelocations),
            uint16_t(Seg.Flags));
  EXPECT_EQ(0x2000u, uint16_t(Seg.MinimumAllocation));
}

static std::array<char, 0xc0> makeNamedNEImage() {
  std::array<char, 0xc0> Data{};
  const auto Base = makeNEImage();
  std::memcpy(Data.data(), Base.data(), Base.size());

  NE::Header Hdr;
  std::memcpy(&Hdr, Data.data() + 0x40, sizeof(Hdr));
  Hdr.ResidentNameTableOffset = 0x48;
  Hdr.ModuleReferenceTableOffset = 0x60;
  Hdr.NonResidentNameTableOffset = 0xb0;
  Hdr.NonResidentNameTableSize = 9;
  std::memcpy(Data.data() + 0x40, &Hdr, sizeof(Hdr));

  auto writeName = [&](size_t &Offset, StringRef Name, uint16_t Ordinal) {
    Data[Offset++] = char(Name.size());
    for (char C : Name)
      Data[Offset++] = C;
    Data[Offset++] = char(Ordinal & 0xff);
    Data[Offset++] = char(Ordinal >> 8);
  };

  size_t Resident = 0x88;
  writeName(Resident, "MYDLL", 0);
  writeName(Resident, "DoIt", 7);
  Data[Resident] = 0;

  size_t NonResident = 0xb0;
  writeName(NonResident, "Extra", 9);
  Data[NonResident] = 0;
  return Data;
}

TEST(NEFileTest, ReadsNamedOrdinals) {
  auto Data = makeNamedNEImage();
  MemoryBufferRef Buffer(StringRef(Data.data(), Data.size()), "named.ne");
  std::unique_ptr<NEFile> Obj = cantFail(NEFile::create(Buffer));

  auto Resident = cantFail(Obj->residentNames());
  ASSERT_EQ(2u, Resident.size());
  EXPECT_EQ("MYDLL", Resident[0].Name);
  EXPECT_EQ(0u, Resident[0].Ordinal);
  EXPECT_EQ("DoIt", Resident[1].Name);
  EXPECT_EQ(7u, Resident[1].Ordinal);

  auto NonResident = cantFail(Obj->nonResidentNames());
  ASSERT_EQ(1u, NonResident.size());
  EXPECT_EQ("Extra", NonResident[0].Name);
  EXPECT_EQ(9u, NonResident[0].Ordinal);
}

TEST(NEFileTest, RejectsMalformedNameTables) {
  {
    auto Data = makeNamedNEImage();
    Data[0x88] = 0x1f; // Entry would cross the module reference table.
    MemoryBufferRef Buffer(StringRef(Data.data(), Data.size()), "bad-resident.ne");
    std::unique_ptr<NEFile> Obj = cantFail(NEFile::create(Buffer));
    auto Names = Obj->residentNames();
    EXPECT_FALSE(Names);
    consumeError(Names.takeError());
  }

  {
    auto Data = makeNamedNEImage();
    NE::Header Hdr;
    std::memcpy(&Hdr, Data.data() + 0x40, sizeof(Hdr));
    Hdr.NonResidentNameTableOffset = 0xbe;
    std::memcpy(Data.data() + 0x40, &Hdr, sizeof(Hdr));
    MemoryBufferRef Buffer(StringRef(Data.data(), Data.size()), "bad-nonresident.ne");
    std::unique_ptr<NEFile> Obj = cantFail(NEFile::create(Buffer));
    auto Names = Obj->nonResidentNames();
    EXPECT_FALSE(Names);
    consumeError(Names.takeError());
  }
}

TEST(NEFileTest, RejectsTruncatedSegmentTable) {
  auto Data = makeNEImage();
  MemoryBufferRef Buffer(StringRef(Data.data(), 0x80), "truncated.ne");
  Expected<std::unique_ptr<NEFile>> Obj = NEFile::create(Buffer);
  EXPECT_FALSE(Obj);
  consumeError(Obj.takeError());
}

} // namespace
