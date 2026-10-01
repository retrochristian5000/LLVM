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

  auto *Hdr = reinterpret_cast<NE::Header *>(Data.data() + 0x40);
  Hdr->Signature = 0x454e;
  Hdr->LinkerVersion = 5;
  Hdr->LinkerRevision = 1;
  Hdr->SegmentTableOffset = sizeof(NE::Header);
  Hdr->SegmentCount = 1;
  Hdr->SegmentAlignmentShift = 4;
  Hdr->TargetOS = 2;

  auto *Seg =
      reinterpret_cast<NE::Segment *>(Data.data() + 0x40 + sizeof(NE::Header));
  Seg->DataOffset = 0x20;
  Seg->DataLength = 0x1234;
  Seg->Flags = NE::SegmentData | NE::SegmentRelocations;
  Seg->MinimumAllocation = 0x2000;
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

TEST(NEFileTest, RejectsTruncatedSegmentTable) {
  auto Data = makeNEImage();
  MemoryBufferRef Buffer(StringRef(Data.data(), 0x80), "truncated.ne");
  Expected<std::unique_ptr<NEFile>> Obj = NEFile::create(Buffer);
  EXPECT_FALSE(Obj);
  consumeError(Obj.takeError());
}

} // namespace
