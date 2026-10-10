//===- llvm/BinaryFormat/MZ.h - DOS MZ executable format ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_BINARYFORMAT_MZ_H
#define LLVM_BINARYFORMAT_MZ_H

#include "llvm/Support/Endian.h"

namespace llvm {
namespace MZ {
// The original DOS EXE header (28 bytes); e_lfanew is a later extension.
struct Header {
  char Magic[2];
  support::ulittle16_t BytesInLastPage;
  support::ulittle16_t PagesInFile;
  support::ulittle16_t RelocationCount;
  support::ulittle16_t HeaderParagraphs;
  support::ulittle16_t MinimumExtraParagraphs;
  support::ulittle16_t MaximumExtraParagraphs;
  support::ulittle16_t InitialSS;
  support::ulittle16_t InitialSP;
  support::ulittle16_t Checksum;
  support::ulittle16_t InitialIP;
  support::ulittle16_t InitialCS;
  support::ulittle16_t RelocationTableOffset;
  support::ulittle16_t OverlayNumber;
};
static_assert(sizeof(Header) == 28, "invalid DOS MZ header size");

struct Relocation {
  support::ulittle16_t Offset;
  support::ulittle16_t Segment;
};
static_assert(sizeof(Relocation) == 4, "invalid DOS MZ relocation size");
} // namespace MZ
} // namespace llvm

#endif // LLVM_BINARYFORMAT_MZ_H
