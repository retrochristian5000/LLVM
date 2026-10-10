//===- NEFile.cpp - Windows NE binary support ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Object/NEFile.h"
#include "llvm/Object/Error.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Endian.h"
#include <algorithm>
#include <cstring>
#include <utility>

using namespace llvm;
using namespace llvm::object;

NEFile::NEFile(MemoryBufferRef Source, uint32_t HeaderOffset,
               NE::Header Hdr, std::vector<NE::Segment> Segments)
    : Binary(Binary::ID_NE, Source), HeaderOffset(HeaderOffset), Hdr(Hdr),
      Segments(std::move(Segments)) {}

Expected<std::unique_ptr<NEFile>> NEFile::create(MemoryBufferRef Source) {
  StringRef Buffer = Source.getBuffer();
  if (Buffer.size() < 0x40)
    return make_error<GenericBinaryError>(
        Source.getBufferIdentifier() + ": too small to contain an MZ header",
        object_error::unexpected_eof);

  if (Buffer[0] != 'M' || Buffer[1] != 'Z')
    return make_error<GenericBinaryError>(
        Source.getBufferIdentifier() + ": missing MZ signature",
        object_error::invalid_file_type);

  uint32_t HeaderOffset =
      support::endian::read32le(Buffer.data() + 0x3c);
  if (HeaderOffset > Buffer.size() ||
      Buffer.size() - HeaderOffset < sizeof(NE::Header))
    return make_error<GenericBinaryError>(
        Source.getBufferIdentifier() + ": truncated NE header",
        object_error::unexpected_eof);

  NE::Header Hdr;
  std::memcpy(&Hdr, Buffer.data() + HeaderOffset, sizeof(Hdr));
  if (uint16_t(Hdr.Signature) != 0x454e)
    return make_error<GenericBinaryError>(
        Source.getBufferIdentifier() + ": invalid NE signature",
        object_error::invalid_file_type);

  uint64_t SegmentOffset =
      uint64_t(HeaderOffset) + uint16_t(Hdr.SegmentTableOffset);
  uint64_t SegmentCount = uint16_t(Hdr.SegmentCount);
  uint64_t SegmentSize = SegmentCount * sizeof(NE::Segment);
  if (SegmentOffset > Buffer.size() ||
      SegmentSize > Buffer.size() - SegmentOffset)
    return make_error<GenericBinaryError>(
        Source.getBufferIdentifier() + ": truncated NE segment table",
        object_error::unexpected_eof);

  std::vector<NE::Segment> Segments(SegmentCount);
  if (SegmentSize)
    std::memcpy(Segments.data(), Buffer.data() + SegmentOffset, SegmentSize);
  return std::unique_ptr<NEFile>(
      new NEFile(Source, HeaderOffset, Hdr, std::move(Segments)));
}

namespace {

// Names in both NE name tables are counted strings followed by a 16-bit
// ordinal, terminated by a zero-length string. Never read across the table
// boundary: malformed tables must not be treated as valid DLL exports.
static Expected<std::vector<NEFile::NameEntry>>
readNameTable(StringRef Data, uint64_t Offset, uint64_t End, StringRef Name) {
  auto invalid = [&](StringRef Reason) -> Error {
    return make_error<GenericBinaryError>(
        Twine(Name) + ": malformed NE name table: " + Reason,
        object_error::parse_failed);
  };

  if (Offset > End || End > Data.size())
    return invalid("offset or size exceeds file");

  std::vector<NEFile::NameEntry> Entries;
  while (Offset < End) {
    uint8_t Length = static_cast<uint8_t>(Data[Offset++]);
    if (!Length)
      return Entries;
    if (uint64_t(Length) + 2 > End - Offset)
      return invalid("truncated name or ordinal");
    Entries.push_back({Data.substr(Offset, Length).str(),
                       support::endian::read16le(Data.data() + Offset + Length)});
    Offset += Length + 2;
  }
  return invalid("missing name-table terminator");
}

} // namespace

Expected<std::vector<NEFile::NameEntry>> NEFile::residentNames() const {
  uint64_t RelativeOffset = uint16_t(Hdr.ResidentNameTableOffset);
  if (!RelativeOffset)
    return std::vector<NameEntry>{};

  uint64_t End = getData().size();
  // Resident names precede the module references, imported names, and entry
  // table. Use the first following table as an upper bound when available.
  for (uint16_t Next : {uint16_t(Hdr.ModuleReferenceTableOffset),
                        uint16_t(Hdr.ImportedNameTableOffset),
                        uint16_t(Hdr.EntryTableOffset)}) {
    if (Next > RelativeOffset)
      End = std::min(End, uint64_t(HeaderOffset) + Next);
  }
  return readNameTable(getData(), uint64_t(HeaderOffset) + RelativeOffset,
                       End, getFileName());
}

Expected<std::vector<NEFile::NameEntry>> NEFile::nonResidentNames() const {
  uint64_t Size = uint16_t(Hdr.NonResidentNameTableSize);
  if (!Size)
    return std::vector<NameEntry>{};

  // Unlike other NE offsets, the non-resident name table uses a file offset.
  uint64_t Offset = uint32_t(Hdr.NonResidentNameTableOffset);
  return readNameTable(getData(), Offset, Offset + Size, getFileName());
}
