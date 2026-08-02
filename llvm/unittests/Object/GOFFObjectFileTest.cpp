//===- GOFFObjectFileTest.cpp - Tests for GOFFObjectFile ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Object/GOFFObjectFile.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Testing/Support/Error.h"
#include "gtest/gtest.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::GOFF;

namespace {
char GOFFData[GOFF::RecordLength * 3] = {0x00};

void constructValidGOFF(size_t Size) {
  StringRef ValidSize(GOFFData, Size);
  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(ValidSize, "dummyGOFF"));

  ASSERT_THAT_EXPECTED(GOFFObjOrErr, Succeeded());
}

void constructInvalidGOFF(size_t Size) {
  // Construct GOFFObject with record of length != multiple of 80.
  StringRef InvalidData(GOFFData, Size);
  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(InvalidData, "dummyGOFF"));

  ASSERT_THAT_EXPECTED(
      GOFFObjOrErr,
      FailedWithMessage("object file is not the right size. Must be a multiple "
                        "of 80 bytes, but is " +
                        std::to_string(Size) + " bytes"));
}
} // namespace

TEST(GOFFObjectFileTest, createObjectFile) {
  const uint8_t GOFFData[] = {
      0x03, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x40, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
  };
  ArrayRef<uint8_t> GOFFRef(GOFFData, sizeof(GOFFData));
  Expected<std::unique_ptr<ObjectFile>> XCOFFObjOrErr =
      object::ObjectFile::createObjectFile(
          MemoryBufferRef(toStringRef(GOFFRef), "dummyGOFF"),
          file_magic::goff_object);
  ASSERT_THAT_EXPECTED(XCOFFObjOrErr, Succeeded());
}

TEST(GOFFObjectFileTest, ConstructGOFFObjectValidSize) {
  GOFFData[0] = (char)0x03;
  GOFFData[1] = (char)0xF0;
  GOFFData[80] = (char)0x03;
  GOFFData[81] = (char)0x40;
  constructValidGOFF(160);
  constructValidGOFF(0);
}

TEST(GOFFObjectFileTest, ConstructGOFFObjectInvalidSize) {
  constructInvalidGOFF(70);
  constructInvalidGOFF(79);
  constructInvalidGOFF(81);
}

TEST(GOFFObjectFileTest, MissingHDR) {
  char GOFFData[GOFF::RecordLength * 2] = {0x00};

  // ESD record.
  GOFFData[0] = (char)0x03;

  // END record.
  GOFFData[GOFF::RecordLength] = (char)0x03;
  GOFFData[GOFF::RecordLength + 1] = (char)0x40;

  StringRef Data(GOFFData, GOFF::RecordLength * 2);

  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(Data, "dummyGOFF"));

  ASSERT_THAT_EXPECTED(
      GOFFObjOrErr,
      FailedWithMessage("object file must start with HDR record"));
}

TEST(GOFFObjectFileTest, MissingEND) {
  char GOFFData[GOFF::RecordLength * 2] = {0x00};

  // HDR record.
  GOFFData[0] = (char)0x03;
  GOFFData[1] = (char)0xF0;

  // ESD record.
  GOFFData[GOFF::RecordLength] = (char)0x03;

  StringRef Data(GOFFData, GOFF::RecordLength * 2);

  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(Data, "dummyGOFF"));

  ASSERT_THAT_EXPECTED(
      GOFFObjOrErr, FailedWithMessage("object file must end with END record"));
}

TEST(GOFFObjectFileTest, GetSymbolName) {
  char GOFFData[GOFF::RecordLength * 3] = {0x00};

  // HDR record.
  GOFFData[0] = (char)0x03;
  GOFFData[1] = (char)0xF0;

  // ESD record.
  GOFFData[GOFF::RecordLength] = (char)0x03;
  GOFFData[GOFF::RecordLength + 3] = (char)0x02;
  GOFFData[GOFF::RecordLength + 7] = (char)0x01;
  GOFFData[GOFF::RecordLength + 11] = (char)0x01;
  GOFFData[GOFF::RecordLength + 71] = (char)0x05; // Size of symbol name.
  GOFFData[GOFF::RecordLength + 72] = (char)0xC8; // Symbol name is Hello.
  GOFFData[GOFF::RecordLength + 73] = (char)0x85;
  GOFFData[GOFF::RecordLength + 74] = (char)0x93;
  GOFFData[GOFF::RecordLength + 75] = (char)0x93;
  GOFFData[GOFF::RecordLength + 76] = (char)0x96;

  // END record.
  GOFFData[GOFF::RecordLength * 2] = 0x03;
  GOFFData[GOFF::RecordLength * 2 + 1] = 0x40;

  StringRef Data(GOFFData, GOFF::RecordLength * 3);

  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(Data, "dummyGOFF"));

  ASSERT_THAT_EXPECTED(GOFFObjOrErr, Succeeded());

  GOFFObjectFile *GOFFObj = dyn_cast<GOFFObjectFile>((*GOFFObjOrErr).get());

  for (SymbolRef Symbol : GOFFObj->symbols()) {
    Expected<StringRef> SymbolNameOrErr = GOFFObj->getSymbolName(Symbol);
    ASSERT_THAT_EXPECTED(SymbolNameOrErr, Succeeded());
    StringRef SymbolName = SymbolNameOrErr.get();

    EXPECT_EQ(SymbolName, "Hello");
  }
}

TEST(GOFFObjectFileTest, ConcatenatedGOFFFile) {
  char GOFFData[GOFF::RecordLength * 6] = {0x00};

  // HDR record.
  GOFFData[0] = (char)0x03;
  GOFFData[1] = (char)0xF0;
  // ESD record.
  GOFFData[GOFF::RecordLength] = (char)0x03;
  // END record.
  GOFFData[GOFF::RecordLength * 2] = (char)0x03;
  GOFFData[GOFF::RecordLength * 2 + 1] = (char)0x40;
  // HDR record.
  GOFFData[GOFF::RecordLength * 3] = (char)0x03;
  GOFFData[GOFF::RecordLength * 3 + 1] = (char)0xF0;
  // ESD record.
  GOFFData[GOFF::RecordLength * 4] = (char)0x03;
  // END record.
  GOFFData[GOFF::RecordLength * 5] = (char)0x03;
  GOFFData[GOFF::RecordLength * 5 + 1] = (char)0x40;

  StringRef Data(GOFFData, GOFF::RecordLength * 6);

  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(Data, "dummyGOFF"));

  ASSERT_THAT_EXPECTED(GOFFObjOrErr, Succeeded());
}

TEST(GOFFObjectFileTest, ContinuationGetSymbolName) {
  char GOFFContData[GOFF::RecordLength * 4] = {0x00};

  // HDR record.
  GOFFContData[0] = (char)0x03;
  GOFFContData[1] = (char)0xF0;

  // ESD record.
  GOFFContData[GOFF::RecordLength] = (char)0x03;
  GOFFContData[GOFF::RecordLength + 1] = (char)0x01;
  GOFFContData[GOFF::RecordLength + 3] = (char)0x02;
  GOFFContData[GOFF::RecordLength + 7] = (char)0x01;
  GOFFContData[GOFF::RecordLength + 11] = (char)0x01;
  GOFFContData[GOFF::RecordLength + 71] = (char)0x0A; // Size of symbol name.
  GOFFContData[GOFF::RecordLength + 72] = (char)0xC8; // Symbol name is HelloWorld.
  GOFFContData[GOFF::RecordLength + 73] = (char)0x85;
  GOFFContData[GOFF::RecordLength + 74] = (char)0x93;
  GOFFContData[GOFF::RecordLength + 75] = (char)0x93;
  GOFFContData[GOFF::RecordLength + 76] = (char)0x96;
  GOFFContData[GOFF::RecordLength + 77] = (char)0xA6;
  GOFFContData[GOFF::RecordLength + 78] = (char)0x96;
  GOFFContData[GOFF::RecordLength + 79] = (char)0x99;

  // ESD continuation record.
  GOFFContData[GOFF::RecordLength * 2] = (char)0x03;
  GOFFContData[GOFF::RecordLength * 2 + 1] = (char)0x02; // No further continuations.
  GOFFContData[GOFF::RecordLength * 2 + 3] = (char)0x93;
  GOFFContData[GOFF::RecordLength * 2 + 4] = (char)0x84;

  // END record.
  GOFFContData[GOFF::RecordLength * 3] = (char)0x03;
  GOFFContData[GOFF::RecordLength * 3 + 1] = (char)0x40;

  StringRef Data(GOFFContData, GOFF::RecordLength * 4);

  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(Data, "dummyGOFF"));

  ASSERT_THAT_EXPECTED(GOFFObjOrErr, Succeeded());

  GOFFObjectFile *GOFFObj = dyn_cast<GOFFObjectFile>((*GOFFObjOrErr).get());

  for (SymbolRef Symbol : GOFFObj->symbols()) {
    Expected<StringRef> SymbolNameOrErr = GOFFObj->getSymbolName(Symbol);
    ASSERT_THAT_EXPECTED(SymbolNameOrErr, Succeeded());
    StringRef SymbolName = SymbolNameOrErr.get();
    EXPECT_EQ(SymbolName, "Helloworld");
  }
}

TEST(GOFFObjectFileTest, ContinuationBitNotSet) {
  char GOFFContData[GOFF::RecordLength * 4] = {0x00};

  // HDR record.
  GOFFContData[0] = (char)0x03;
  GOFFContData[1] = (char)0xF0;

  // ESD record.
  GOFFContData[GOFF::RecordLength] = (char)0x03;
  GOFFContData[GOFF::RecordLength + 1] = (char)0x01;
  GOFFContData[GOFF::RecordLength + 3] = (char)0x02;
  GOFFContData[GOFF::RecordLength + 7] = (char)0x01;
  GOFFContData[GOFF::RecordLength + 11] = (char)0x01;
  GOFFContData[GOFF::RecordLength + 71] = (char)0x0A; // Size of symbol name.
  GOFFContData[GOFF::RecordLength + 72] = (char)0xC8; // Symbol name is HelloWorld.
  GOFFContData[GOFF::RecordLength + 73] = (char)0x85;
  GOFFContData[GOFF::RecordLength + 74] = (char)0x93;
  GOFFContData[GOFF::RecordLength + 75] = (char)0x93;
  GOFFContData[GOFF::RecordLength + 76] = (char)0x96;
  GOFFContData[GOFF::RecordLength + 77] = (char)0xA6;
  GOFFContData[GOFF::RecordLength + 78] = (char)0x96;
  GOFFContData[GOFF::RecordLength + 79] = (char)0x99;

  // ESD continuation record.
  GOFFContData[GOFF::RecordLength * 2] = (char)0x03;
  GOFFContData[GOFF::RecordLength * 2 + 1] = (char)0x00;
  GOFFContData[GOFF::RecordLength * 2 + 3] = (char)0x93;
  GOFFContData[GOFF::RecordLength * 2 + 4] = (char)0x84;

  // END record.
  GOFFContData[GOFF::RecordLength * 3] = (char)0x03;
  GOFFContData[GOFF::RecordLength * 3 + 1] = (char)0x40;

  StringRef Data(GOFFContData, GOFF::RecordLength * 4);

  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(Data, "dummyGOFF"));
  EXPECT_THAT_EXPECTED(
      GOFFObjOrErr,
      FailedWithMessage("record 2 is not a continuation record but the "
                        "preceding record is continued"));
}

TEST(GOFFObjectFileTest, ContinuationRecordNotTerminated) {
  char GOFFContData[GOFF::RecordLength * 4] = {0x00};

  // HDR record.
  GOFFContData[0] = (char)0x03;
  GOFFContData[1] = (char)0xF0;

  // ESD record.
  GOFFContData[GOFF::RecordLength] = (char)0x03;
  GOFFContData[GOFF::RecordLength + 1] = (char)0x01;
  GOFFContData[GOFF::RecordLength + 3] = (char)0x02;
  GOFFContData[GOFF::RecordLength + 7] = (char)0x01;
  GOFFContData[GOFF::RecordLength + 11] = (char)0x01;
  GOFFContData[GOFF::RecordLength + 71] = (char)0x0A; // Size of symbol name.
  GOFFContData[GOFF::RecordLength + 72] = (char)0xC8; // Symbol name is HelloWorld.
  GOFFContData[GOFF::RecordLength + 73] = (char)0x85;
  GOFFContData[GOFF::RecordLength + 74] = (char)0x93;
  GOFFContData[GOFF::RecordLength + 75] = (char)0x93;
  GOFFContData[GOFF::RecordLength + 76] = (char)0x96;
  GOFFContData[GOFF::RecordLength + 77] = (char)0xA6;
  GOFFContData[GOFF::RecordLength + 78] = (char)0x96;
  GOFFContData[GOFF::RecordLength + 79] = (char)0x99;

  // ESD continuation record.
  GOFFContData[GOFF::RecordLength * 2] = (char)0x03;
  GOFFContData[GOFF::RecordLength * 2 + 1] = (char)0x03; // Continued bit set.
  GOFFContData[GOFF::RecordLength * 2 + 3] = (char)0x93;
  GOFFContData[GOFF::RecordLength * 2 + 4] = (char)0x84;

  // END record.
  GOFFContData[GOFF::RecordLength * 3] = (char)0x03;
  GOFFContData[GOFF::RecordLength * 3 + 1] = (char)0x40;

  StringRef Data(GOFFContData, GOFF::RecordLength * 4);

  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(Data, "dummyGOFF"));
  ASSERT_THAT_EXPECTED(GOFFObjOrErr, Succeeded());

  GOFFObjectFile *GOFFObj = dyn_cast<GOFFObjectFile>((*GOFFObjOrErr).get());

  for (SymbolRef Symbol : GOFFObj->symbols()) {
    Expected<StringRef> SymbolNameOrErr = GOFFObj->getSymbolName(Symbol);
    EXPECT_THAT_EXPECTED(SymbolNameOrErr,
                         FailedWithMessage("continued bit should not be set"));
  }
}

TEST(GOFFObjectFileTest, PrevNotContinued) {
  char GOFFContData[GOFF::RecordLength * 4] = {0x00};

  // HDR record.
  GOFFContData[0] = (char)0x03;
  GOFFContData[1] = (char)0xF0;

  // ESD record, with continued bit not set.
  GOFFContData[GOFF::RecordLength] = (char)0x03;

  // ESD continuation record.
  GOFFContData[GOFF::RecordLength * 2] = (char)0x03;
  GOFFContData[GOFF::RecordLength * 2 + 1] = (char)0x02;

  // END record.
  GOFFContData[GOFF::RecordLength * 3] = (char)0x03;
  GOFFContData[GOFF::RecordLength * 3 + 1] = (char)0x40;

  StringRef Data(GOFFContData, GOFF::RecordLength * 4);

  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(Data, "dummyGOFF"));

  ASSERT_THAT_EXPECTED(
      GOFFObjOrErr,
      FailedWithMessage("record 2 is a continuation record that is not "
                        "preceded by a continued record"));
}

TEST(GOFFObjectFileTest, ContinuationTypeMismatch) {
  char GOFFContData[GOFF::RecordLength * 4] = {0x00};

  // HDR record.
  GOFFContData[0] = (char)0x03;
  GOFFContData[1] = (char)0xF0;

  // ESD record.
  GOFFContData[GOFF::RecordLength] = (char)0x03;
  GOFFContData[GOFF::RecordLength + 1] = (char)0x01; // Continued to next record.

  // END continuation record.
  GOFFContData[GOFF::RecordLength * 2] = (char)0x03;
  GOFFContData[GOFF::RecordLength * 2 + 1] = (char)0x42;

  // END record.
  GOFFContData[GOFF::RecordLength * 3] = (char)0x03;
  GOFFContData[GOFF::RecordLength * 3 + 1] = (char)0x40;

  StringRef Data(GOFFContData, GOFF::RecordLength * 4);

  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(Data, "dummyGOFF"));

  ASSERT_THAT_EXPECTED(
      GOFFObjOrErr,
      FailedWithMessage("record 2 is a continuation record that does not match "
                        "the type of the previous record"));
}

TEST(GOFFObjectFileTest, TwoSymbols) {
  char GOFFData[GOFF::RecordLength * 4] = {0x00};

  // HDR record.
  GOFFData[0] = (char)0x03;
  GOFFData[1] = (char)0xF0;

  // ESD record 1.
  GOFFData[GOFF::RecordLength] = (char)0x03;
  GOFFData[GOFF::RecordLength + 3] = (char)0x00;
  GOFFData[GOFF::RecordLength + 7] = (char)0x01;  // ESDID.
  GOFFData[GOFF::RecordLength + 71] = (char)0x01; // Size of symbol name.
  GOFFData[GOFF::RecordLength + 72] = (char)0xa7; // Symbol name is x.

  // ESD record 2.
  GOFFData[GOFF::RecordLength * 2] = (char)0x03;
  GOFFData[GOFF::RecordLength * 2 + 3] = (char)0x03;
  GOFFData[GOFF::RecordLength * 2 + 7] = (char)0x02;  // ESDID.
  GOFFData[GOFF::RecordLength * 2 + 11] = (char)0x01; // Parent ESDID.
  GOFFData[GOFF::RecordLength * 2 + 71] = (char)0x05; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 2 + 72] = (char)0xC8; // Symbol name is Hello.
  GOFFData[GOFF::RecordLength * 2 + 73] = (char)0x85;
  GOFFData[GOFF::RecordLength * 2 + 74] = (char)0x93;
  GOFFData[GOFF::RecordLength * 2 + 75] = (char)0x93;
  GOFFData[GOFF::RecordLength * 2 + 76] = (char)0x96;

  // END record.
  GOFFData[GOFF::RecordLength * 3] = (char)0x03;
  GOFFData[GOFF::RecordLength * 3 + 1] = (char)0x40;

  StringRef Data(GOFFData, GOFF::RecordLength * 4);

  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(Data, "dummyGOFF"));

  ASSERT_THAT_EXPECTED(GOFFObjOrErr, Succeeded());

  GOFFObjectFile *GOFFObj = dyn_cast<GOFFObjectFile>((*GOFFObjOrErr).get());

  for (SymbolRef Symbol : GOFFObj->symbols()) {
    Expected<StringRef> SymbolNameOrErr = GOFFObj->getSymbolName(Symbol);
    ASSERT_THAT_EXPECTED(SymbolNameOrErr, Succeeded());
    StringRef SymbolName = SymbolNameOrErr.get();
    EXPECT_EQ(SymbolName, "Hello");
  }
}

TEST(GOFFObjectFileTest, InvalidSymbolType) {
  char GOFFData[GOFF::RecordLength * 3] = {0x00};

  // HDR record.
  GOFFData[0] = (char)0x03;
  GOFFData[1] = (char)0xF0;

  // ESD record.
  GOFFData[GOFF::RecordLength] = (char)0x03;
  GOFFData[GOFF::RecordLength + 3] = (char)0x05;
  GOFFData[GOFF::RecordLength + 7] = (char)0x01;
  GOFFData[GOFF::RecordLength + 11] = (char)0x01;
  GOFFData[GOFF::RecordLength + 71] = (char)0x01; // Size of symbol name.
  GOFFData[GOFF::RecordLength + 72] = (char)0xC8; // Symbol name.

  // END record.
  GOFFData[GOFF::RecordLength * 2] = (char)0x03;
  GOFFData[GOFF::RecordLength * 2 + 1] = (char)0x40;

  StringRef Data(GOFFData, GOFF::RecordLength * 3);

  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(Data, "dummyGOFF"));

  ASSERT_THAT_EXPECTED(GOFFObjOrErr, Succeeded());

  GOFFObjectFile *GOFFObj = dyn_cast<GOFFObjectFile>((*GOFFObjOrErr).get());

  for (SymbolRef Symbol : GOFFObj->symbols()) {
    Expected<SymbolRef::Type> SymbolType = Symbol.getType();
    EXPECT_THAT_EXPECTED(
        SymbolType,
        FailedWithMessage("ESD record 1 has invalid symbol type 0x05"));

    Expected<section_iterator> SymSI = Symbol.getSection();
    ASSERT_THAT_EXPECTED(
        SymSI,
        FailedWithMessage(
            "symbol with ESD id 1 refers to invalid section with ESD id 1"));
  }
}

TEST(GOFFObjectFileTest, InvalidERSymbolType) {
  char GOFFData[GOFF::RecordLength * 3] = {0x00};

  // HDR record.
  GOFFData[0] = (char)0x03;
  GOFFData[1] = (char)0xF0;

  // ESD record.
  GOFFData[GOFF::RecordLength] = (char)0x03;
  GOFFData[GOFF::RecordLength + 3] = (char)0x04;
  GOFFData[GOFF::RecordLength + 7] = (char)0x01;
  GOFFData[GOFF::RecordLength + 11] = (char)0x01;
  GOFFData[GOFF::RecordLength + 63] = (char)0x03; // Unknown executable type.
  GOFFData[GOFF::RecordLength + 71] = (char)0x01; // Size of symbol name.
  GOFFData[GOFF::RecordLength + 72] = (char)0xC8; // Symbol name.

  // END record.
  GOFFData[GOFF::RecordLength * 2] = (char)0x03;
  GOFFData[GOFF::RecordLength * 2 + 1] = (char)0x40;

  StringRef Data(GOFFData, GOFF::RecordLength * 3);

  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(Data, "dummyGOFF"));

  ASSERT_THAT_EXPECTED(GOFFObjOrErr, Succeeded());

  GOFFObjectFile *GOFFObj = dyn_cast<GOFFObjectFile>((*GOFFObjOrErr).get());

  for (SymbolRef Symbol : GOFFObj->symbols()) {
    Expected<SymbolRef::Type> SymbolType = Symbol.getType();
    EXPECT_THAT_EXPECTED(
        SymbolType,
        FailedWithMessage("ESD record 1 has unknown Executable type 0x03"));
  }
}

TEST(GOFFObjectFileTest, TXTConstruct) {
  char GOFFData[GOFF::RecordLength * 6] = {};

  // HDR record.
  GOFFData[0] = (char)0x03;
  GOFFData[1] = (char)0xF0;
  GOFFData[50] = (char)0x01;

  // ESD record.
  GOFFData[GOFF::RecordLength] = (char)0x03;
  GOFFData[GOFF::RecordLength + 7] = (char)0x01;  // ESDID.
  GOFFData[GOFF::RecordLength + 71] = (char)0x05; // Size of symbol name.
  GOFFData[GOFF::RecordLength + 72] = (char)0xa5; // Symbol name is v.
  GOFFData[GOFF::RecordLength + 73] = (char)0x81; // Symbol name is a.
  GOFFData[GOFF::RecordLength + 74] = (char)0x99; // Symbol name is r.
  GOFFData[GOFF::RecordLength + 75] = (char)0x7b; // Symbol name is #.
  GOFFData[GOFF::RecordLength + 76] = (char)0x83; // Symbol name is c.

  // ESD record.
  GOFFData[GOFF::RecordLength * 2] = (char)0x03;
  GOFFData[GOFF::RecordLength * 2 + 3] = (char)0x01;
  GOFFData[GOFF::RecordLength * 2 + 7] = (char)0x02;  // ESDID.
  GOFFData[GOFF::RecordLength * 2 + 11] = (char)0x01; // Parent ESDID.
  GOFFData[GOFF::RecordLength * 2 + 27] = (char)0x08; // Length.
  GOFFData[GOFF::RecordLength * 2 + 40] = (char)0x01; // Name Space ID.
  GOFFData[GOFF::RecordLength * 2 + 41] = (char)0x80;
  GOFFData[GOFF::RecordLength * 2 + 60] = (char)0x04; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 2 + 61] = (char)0x04; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 2 + 63] = (char)0x0a; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 2 + 66] = (char)0x03; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 2 + 71] = (char)0x08; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 2 + 72] = (char)0xc3; // Symbol name is c.
  GOFFData[GOFF::RecordLength * 2 + 73] = (char)0x6d; // Symbol name is _.
  GOFFData[GOFF::RecordLength * 2 + 74] = (char)0xc3; // Symbol name is c.
  GOFFData[GOFF::RecordLength * 2 + 75] = (char)0xd6; // Symbol name is o.
  GOFFData[GOFF::RecordLength * 2 + 76] = (char)0xc4; // Symbol name is D.
  GOFFData[GOFF::RecordLength * 2 + 77] = (char)0xc5; // Symbol name is E.
  GOFFData[GOFF::RecordLength * 2 + 78] = (char)0xf6; // Symbol name is 6.
  GOFFData[GOFF::RecordLength * 2 + 79] = (char)0xf4; // Symbol name is 4.

  // ESD record.
  GOFFData[GOFF::RecordLength * 3] = (char)0x03;
  GOFFData[GOFF::RecordLength * 3 + 3] = (char)0x02;
  GOFFData[GOFF::RecordLength * 3 + 7] = (char)0x03;  // ESDID.
  GOFFData[GOFF::RecordLength * 3 + 11] = (char)0x02; // Parent ESDID.
  GOFFData[GOFF::RecordLength * 3 + 71] = (char)0x05; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 3 + 72] = (char)0xa5; // Symbol name is v.
  GOFFData[GOFF::RecordLength * 3 + 73] = (char)0x81; // Symbol name is a.
  GOFFData[GOFF::RecordLength * 3 + 74] = (char)0x99; // Symbol name is r.
  GOFFData[GOFF::RecordLength * 3 + 75] = (char)0x7b; // Symbol name is #.
  GOFFData[GOFF::RecordLength * 3 + 76] = (char)0x83; // Symbol name is c.

  // TXT record.
  GOFFData[GOFF::RecordLength * 4] = (char)0x03;
  GOFFData[GOFF::RecordLength * 4 + 1] = (char)0x10;
  GOFFData[GOFF::RecordLength * 4 + 7] = (char)0x02;
  GOFFData[GOFF::RecordLength * 4 + 23] = (char)0x08; // Data Length.
  GOFFData[GOFF::RecordLength * 4 + 24] = (char)0x12;
  GOFFData[GOFF::RecordLength * 4 + 25] = (char)0x34;
  GOFFData[GOFF::RecordLength * 4 + 26] = (char)0x56;
  GOFFData[GOFF::RecordLength * 4 + 27] = (char)0x78;
  GOFFData[GOFF::RecordLength * 4 + 28] = (char)0x9a;
  GOFFData[GOFF::RecordLength * 4 + 29] = (char)0xbc;
  GOFFData[GOFF::RecordLength * 4 + 30] = (char)0xde;
  GOFFData[GOFF::RecordLength * 4 + 31] = (char)0xf0;

  // END record.
  GOFFData[GOFF::RecordLength * 5] = (char)0x03;
  GOFFData[GOFF::RecordLength * 5 + 1] = (char)0x40;
  GOFFData[GOFF::RecordLength * 5 + 11] = (char)0x06;

  StringRef Data(GOFFData, GOFF::RecordLength * 6);

  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(Data, "dummyGOFF"));

  ASSERT_THAT_EXPECTED(GOFFObjOrErr, Succeeded());

  GOFFObjectFile *GOFFObj = dyn_cast<GOFFObjectFile>((*GOFFObjOrErr).get());
  auto Symbols = GOFFObj->symbols();
  ASSERT_EQ(std::distance(Symbols.begin(), Symbols.end()), 1);
  SymbolRef Symbol = *Symbols.begin();
  Expected<StringRef> SymbolNameOrErr = GOFFObj->getSymbolName(Symbol);
  ASSERT_THAT_EXPECTED(SymbolNameOrErr, Succeeded());
  StringRef SymbolName = SymbolNameOrErr.get();
  EXPECT_EQ(SymbolName, "var#c");

  auto Sections = GOFFObj->sections();
  ASSERT_EQ(std::distance(Sections.begin(), Sections.end()), 1);
  SectionRef Section = *Sections.begin();
  Expected<StringRef> SectionContent = Section.getContents();
  ASSERT_THAT_EXPECTED(SectionContent, Succeeded());
  StringRef Contents = SectionContent.get();
  EXPECT_EQ(Contents, "\x12\x34\x56\x78\x9a\xbc\xde\xf0");
}

TEST(GOFFObjectFileTest, GlobalSymbols) {
  char GOFFData[GOFF::RecordLength * 12] = {0x00};

  // HDR record.
  GOFFData[0] = (char)0x03;
  GOFFData[1] = (char)0xF0;

  // ESD record 1: type SD
  GOFFData[GOFF::RecordLength] = (char)0x03;
  GOFFData[GOFF::RecordLength + 3] = (char)0x00;  // Type: SD
  GOFFData[GOFF::RecordLength + 7] = (char)0x01;  // ESDID.
  GOFFData[GOFF::RecordLength + 71] = (char)0x01; // Size of symbol name.
  GOFFData[GOFF::RecordLength + 72] = (char)0xC1; // Symbol name is A.

  // ESD record 2: type ED
  GOFFData[GOFF::RecordLength * 2] = (char)0x03;
  GOFFData[GOFF::RecordLength * 2 + 3] = (char)0x01;  // Type: ED
  GOFFData[GOFF::RecordLength * 2 + 7] = (char)0x02;  // ESDID.
  GOFFData[GOFF::RecordLength * 2 + 11] = (char)0x01; // Parent ESDID.
  GOFFData[GOFF::RecordLength * 2 + 71] = (char)0x01; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 2 + 72] = (char)0xC2; // Symbol name is B.

  // ESD record 3: type LD
  GOFFData[GOFF::RecordLength * 3] = (char)0x03;
  GOFFData[GOFF::RecordLength * 3 + 3] = (char)0x02;  // Type: LD
  GOFFData[GOFF::RecordLength * 3 + 7] = (char)0x03;  // ESDID.
  GOFFData[GOFF::RecordLength * 3 + 11] = (char)0x02; // Parent ESDID.
  GOFFData[GOFF::RecordLength * 3 + 71] = (char)0x01; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 3 + 72] = (char)0xC3; // Symbol name is C.

  // ESD record 4: type PR
  GOFFData[GOFF::RecordLength * 4] = (char)0x03;
  GOFFData[GOFF::RecordLength * 4 + 3] = (char)0x03;  // Type: PR
  GOFFData[GOFF::RecordLength * 4 + 7] = (char)0x04;  // ESDID.
  GOFFData[GOFF::RecordLength * 4 + 11] = (char)0x02; // Parent ESDID.
  GOFFData[GOFF::RecordLength * 4 + 71] = (char)0x01; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 4 + 72] = (char)0xC4; // Symbol name is D.

  // ESD record 5: type ErWx
  GOFFData[GOFF::RecordLength * 5] = (char)0x03;
  GOFFData[GOFF::RecordLength * 5 + 3] = (char)0x04;  // Type: ErWx
  GOFFData[GOFF::RecordLength * 5 + 7] = (char)0x05;  // ESDID.
  GOFFData[GOFF::RecordLength * 5 + 71] = (char)0x01; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 5 + 72] = (char)0xC5; // Symbol name is E.

  // ESD record 6: type LD + Section binding scope
  GOFFData[GOFF::RecordLength * 6] = (char)0x03;
  GOFFData[GOFF::RecordLength * 6 + 3] = (char)0x02;  // Type: LD
  GOFFData[GOFF::RecordLength * 6 + 7] = (char)0x06;  // ESDID.
  GOFFData[GOFF::RecordLength * 6 + 11] = (char)0x02; // Parent ESDID.
  GOFFData[GOFF::RecordLength * 6 + 65] = (char)0x01; // Binding Scope: Section.
  GOFFData[GOFF::RecordLength * 6 + 71] = (char)0x01; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 6 + 72] = (char)0xC6; // Symbol name is F.

  // ESD record 7: type LD + Module binding scope
  GOFFData[GOFF::RecordLength * 7] = (char)0x03;
  GOFFData[GOFF::RecordLength * 7 + 3] = (char)0x02;  // Type: LD
  GOFFData[GOFF::RecordLength * 7 + 7] = (char)0x07;  // ESDID.
  GOFFData[GOFF::RecordLength * 7 + 11] = (char)0x02; // Parent ESDID.
  GOFFData[GOFF::RecordLength * 7 + 65] = (char)0x02; // Binding Scope: Module.
  GOFFData[GOFF::RecordLength * 7 + 71] = (char)0x01; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 7 + 72] = (char)0xC7; // Symbol name is G.

  // ESD record 8: type LD + Library binding scope
  GOFFData[GOFF::RecordLength * 8] = (char)0x03;
  GOFFData[GOFF::RecordLength * 8 + 3] = (char)0x02;  // Type: LD
  GOFFData[GOFF::RecordLength * 8 + 7] = (char)0x08;  // ESDID.
  GOFFData[GOFF::RecordLength * 8 + 11] = (char)0x02; // Parent ESDID.
  GOFFData[GOFF::RecordLength * 8 + 65] = (char)0x03; // Binding Scope: Library.
  GOFFData[GOFF::RecordLength * 8 + 71] = (char)0x01; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 8 + 72] = (char)0xC8; // Symbol name is H.

  // ESD record 9: type LD + Import-Export binding scope
  GOFFData[GOFF::RecordLength * 9] = (char)0x03;
  GOFFData[GOFF::RecordLength * 9 + 3] = (char)0x02;  // Type: LD
  GOFFData[GOFF::RecordLength * 9 + 7] = (char)0x09;  // ESDID.
  GOFFData[GOFF::RecordLength * 9 + 11] = (char)0x02; // Parent ESDID.
  GOFFData[GOFF::RecordLength * 9 + 65] =
      (char)0x04; // Binding Scope: ImportExport.
  GOFFData[GOFF::RecordLength * 9 + 71] = (char)0x01; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 9 + 72] = (char)0xC9; // Symbol name is I.

  // ESD record 10: type LD + blank name
  GOFFData[GOFF::RecordLength * 10] = (char)0x03;
  GOFFData[GOFF::RecordLength * 10 + 3] = (char)0x02;  // Type: LD
  GOFFData[GOFF::RecordLength * 10 + 7] = (char)0x0A;  // ESDID.
  GOFFData[GOFF::RecordLength * 10 + 11] = (char)0x02; // Parent ESDID.
  GOFFData[GOFF::RecordLength * 10 + 71] = (char)0x01; // Size of symbol name.
  GOFFData[GOFF::RecordLength * 10 + 72] = (char)0x40; // Symbol name is ' '.

  // END record.
  GOFFData[GOFF::RecordLength * 11] = (char)0x03;
  GOFFData[GOFF::RecordLength * 11 + 1] = (char)0x40;

  StringRef Data(GOFFData, GOFF::RecordLength * 12);

  Expected<std::unique_ptr<ObjectFile>> GOFFObjOrErr =
      object::ObjectFile::createGOFFObjectFile(
          MemoryBufferRef(Data, "dummyGOFF"));

  ASSERT_THAT_EXPECTED(GOFFObjOrErr, Succeeded());

  GOFFObjectFile *GOFFObj = static_cast<GOFFObjectFile*>((*GOFFObjOrErr).get());

  auto SymbolRange = GOFFObj->symbols();
  auto Symbol = SymbolRange.begin();
  auto ValidateGlobal = [&](StringRef Name, bool IsGlobal) {
    ASSERT_TRUE(Symbol != SymbolRange.end());

    // Check Name.
    Expected<StringRef> SymbolNameOrErr = GOFFObj->getSymbolName(*Symbol);
    ASSERT_THAT_EXPECTED(SymbolNameOrErr, Succeeded());
    StringRef SymbolName = SymbolNameOrErr.get();
    EXPECT_EQ(SymbolName, Name);

    // Check flags.
    Expected<uint32_t> SymbolFlagsOrErr = Symbol->getFlags();
    ASSERT_THAT_EXPECTED(SymbolFlagsOrErr, Succeeded());
    uint32_t SymbolFlags = SymbolFlagsOrErr.get();
    if (IsGlobal) {
      EXPECT_TRUE(SymbolFlags & SymbolRef::SF_Global);
    } else {
      EXPECT_FALSE(SymbolFlags & SymbolRef::SF_Global);
    }

    ++Symbol;
  };

  // ESD records 'A' and 'B' shouldn't be considered symbols.
  ValidateGlobal("C", true);
  ValidateGlobal("D", true);
  ValidateGlobal("E", true);
  ValidateGlobal("F", false);
  ValidateGlobal("G", false);
  ValidateGlobal("H", true);
  ValidateGlobal("I", true);
  ValidateGlobal(" ", false);
}
