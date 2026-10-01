//===- NEFile.cpp - Windows NE binary support ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Object/NEFile.h"
#include "llvm/Object/Error.h"
#include "llvm/Support/Endian.h"
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
