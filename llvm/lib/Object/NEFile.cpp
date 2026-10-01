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

using namespace llvm;
using namespace llvm::object;

NEFile::NEFile(MemoryBufferRef Source, uint32_t HeaderOffset,
               const NE::Header *Hdr, ArrayRef<NE::Segment> Segments)
    : Binary(Binary::ID_NE, Source), HeaderOffset(HeaderOffset), Hdr(Hdr),
      Segments(Segments) {}

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

  const auto *Hdr =
      reinterpret_cast<const NE::Header *>(Buffer.data() + HeaderOffset);
  if (uint16_t(Hdr->Signature) != 0x454e)
    return make_error<GenericBinaryError>(
        Source.getBufferIdentifier() + ": invalid NE signature",
        object_error::invalid_file_type);

  uint64_t SegmentOffset =
      uint64_t(HeaderOffset) + uint16_t(Hdr->SegmentTableOffset);
  uint64_t SegmentSize =
      uint64_t(uint16_t(Hdr->SegmentCount)) * sizeof(NE::Segment);
  if (SegmentOffset > Buffer.size() ||
      SegmentSize > Buffer.size() - SegmentOffset)
    return make_error<GenericBinaryError>(
        Source.getBufferIdentifier() + ": truncated NE segment table",
        object_error::unexpected_eof);

  const auto *Segments =
      reinterpret_cast<const NE::Segment *>(Buffer.data() + SegmentOffset);
  return std::unique_ptr<NEFile>(
      new NEFile(Source, HeaderOffset, Hdr,
                 ArrayRef<NE::Segment>(Segments, uint16_t(Hdr->SegmentCount))));
}
