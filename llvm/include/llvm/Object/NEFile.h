//===- llvm/Object/NEFile.h - Windows NE binary support ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_OBJECT_NEFILE_H
#define LLVM_OBJECT_NEFILE_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/BinaryFormat/NE.h"
#include "llvm/Object/Binary.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Error.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace llvm {
namespace object {

class LLVM_ABI NEFile : public Binary {
public:
  // NE DLL exports and module names use length-prefixed strings followed
  // by 16-bit ordinals. The module name itself has ordinal zero.
  struct NameEntry {
    std::string Name;
    uint16_t Ordinal;
  };

  static bool classof(const Binary *V) { return V->isNE(); }

  static Expected<std::unique_ptr<NEFile>> create(MemoryBufferRef Source);

  const NE::Header &getHeader() const { return Hdr; }
  uint32_t getHeaderOffset() const { return HeaderOffset; }
  ArrayRef<NE::Segment> segments() const { return Segments; }

  // Return the named ordinal tables for Win16 DLL export resolution.
  // Errors are reported for malformed names, offsets, or unterminated tables.
  Expected<std::vector<NameEntry>> residentNames() const;
  Expected<std::vector<NameEntry>> nonResidentNames() const;

  uint32_t getSegmentAlignmentShift() const {
    return Hdr.SegmentAlignmentShift ? uint16_t(Hdr.SegmentAlignmentShift) : 9;
  }

private:
  NEFile(MemoryBufferRef Source, uint32_t HeaderOffset, NE::Header Hdr,
         std::vector<NE::Segment> Segments);

  uint32_t HeaderOffset;
  NE::Header Hdr;
  std::vector<NE::Segment> Segments;
};

} // namespace object
} // namespace llvm

#endif // LLVM_OBJECT_NEFILE_H
