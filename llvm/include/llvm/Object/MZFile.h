//===- llvm/Object/MZFile.h - DOS MZ executable reader --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_OBJECT_MZFILE_H
#define LLVM_OBJECT_MZFILE_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/BinaryFormat/MZ.h"
#include "llvm/Object/Binary.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Error.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace llvm {
namespace object {

// A real-mode DOS executable is not a COFF ObjectFile.
class LLVM_ABI MZFile : public Binary {
public:
  static bool classof(const Binary *V) { return V->isMZ(); }
  static Expected<std::unique_ptr<MZFile>> create(MemoryBufferRef Source);
  const MZ::Header &getHeader() const { return Hdr; }
  uint32_t getHeaderSize() const { return HeaderSize; }
  uint32_t getDeclaredFileSize() const { return FileSize; }
  ArrayRef<MZ::Relocation> relocations() const { return Relocations; }
  StringRef getLoadImage() const;
  StringRef getOverlay() const;

private:
  MZFile(MemoryBufferRef Source, MZ::Header Hdr, uint32_t HeaderSize,
         uint32_t FileSize, std::vector<MZ::Relocation> Relocations);
  MZ::Header Hdr;
  uint32_t HeaderSize;
  uint32_t FileSize;
  std::vector<MZ::Relocation> Relocations;
};

} // namespace object
} // namespace llvm

#endif // LLVM_OBJECT_MZFILE_H
