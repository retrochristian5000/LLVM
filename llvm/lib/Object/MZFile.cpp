//===- MZFile.cpp - DOS MZ executable reader ------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Object/MZFile.h"
#include "llvm/Object/Error.h"
#include <cstring>
#include <utility>

using namespace llvm;
using namespace llvm::object;

MZFile::MZFile(MemoryBufferRef Source, MZ::Header Hdr, uint32_t HeaderSize,
               uint32_t FileSize, std::vector<MZ::Relocation> Relocations)
    : Binary(Binary::ID_MZ, Source), Hdr(Hdr), HeaderSize(HeaderSize),
      FileSize(FileSize), Relocations(std::move(Relocations)) {}

Expected<std::unique_ptr<MZFile>> MZFile::create(MemoryBufferRef Source) {
  StringRef Data = Source.getBuffer();
  auto invalid = [&](const char *Reason) -> Error {
    return make_error<GenericBinaryError>(
        Source.getBufferIdentifier() + ": " + Reason,
        object_error::parse_failed);
  };
  if (Data.size() < sizeof(MZ::Header))
    return invalid("truncated DOS MZ header");

  MZ::Header Hdr;
  std::memcpy(&Hdr, Data.data(), sizeof(Hdr));
  if (Hdr.Magic[0] != 'M' || Hdr.Magic[1] != 'Z')
    return invalid("missing DOS MZ signature");

  uint32_t Pages = uint16_t(Hdr.PagesInFile);
  uint32_t Last = uint16_t(Hdr.BytesInLastPage);
  if (!Pages || Last >= 512)
    return invalid("invalid DOS MZ page count");

  // A zero final-page byte count represents a full 512-byte page.
  uint64_t FileSize = uint64_t(Pages - 1) * 512 + (Last ? Last : 512);
  uint64_t HeaderSize = uint64_t(uint16_t(Hdr.HeaderParagraphs)) * 16;
  if (HeaderSize < sizeof(MZ::Header) || HeaderSize > FileSize)
    return invalid("invalid DOS MZ header size");
  if (FileSize > Data.size())
    return invalid("truncated DOS MZ image");

  uint64_t RelocOffset = uint16_t(Hdr.RelocationTableOffset);
  uint64_t Count = uint16_t(Hdr.RelocationCount);
  uint64_t RelocSize = Count * sizeof(MZ::Relocation);
  if (Count && (RelocOffset < sizeof(MZ::Header) ||
                RelocOffset > HeaderSize ||
                RelocSize > HeaderSize - RelocOffset))
    return invalid("DOS MZ relocation table exceeds header");

  std::vector<MZ::Relocation> Relocations(Count);
  if (Count)
    std::memcpy(Relocations.data(), Data.data() + RelocOffset, RelocSize);

  return std::unique_ptr<MZFile>(
      new MZFile(Source, Hdr, uint32_t(HeaderSize), uint32_t(FileSize),
                 std::move(Relocations)));
}

StringRef MZFile::getLoadImage() const {
  return getData().substr(HeaderSize, FileSize - HeaderSize);
}
StringRef MZFile::getOverlay() const { return getData().substr(FileSize); }
