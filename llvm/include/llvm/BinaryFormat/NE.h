//===- llvm/BinaryFormat/NE.h - Windows NE format --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Definitions for the Windows New Executable (NE) format used by 16-bit
// Windows modules.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_BINARYFORMAT_NE_H
#define LLVM_BINARYFORMAT_NE_H

#include "llvm/Support/Endian.h"
#include <cstdint>

namespace llvm {
namespace NE {

static constexpr char Magic[] = {'N', 'E'};

enum HeaderFlags : uint16_t {
  SingleData = 0x0001,
  LibraryModule = 0x8000,
};

enum SegmentFlags : uint16_t {
  SegmentData = 0x0001,
  SegmentAllocated = 0x0002,
  SegmentLoaded = 0x0004,
  SegmentIterated = 0x0008,
  SegmentMoveable = 0x0010,
  SegmentShareable = 0x0020,
  SegmentPreload = 0x0040,
  SegmentExecuteOnly = 0x0080,
  SegmentReadOnly = 0x0080,
  SegmentRelocations = 0x0100,
  SegmentSelfLoad = 0x0800,
  SegmentDiscardable = 0x1000,
  Segment32Bit = 0x2000,
};

enum RelocationAddressType : uint8_t {
  RelocLowByte = 0,
  RelocSelector = 2,
  RelocPointer32 = 3,
  RelocOffset16 = 5,
  RelocPointer48 = 11,
  RelocOffset32 = 13,
};

enum RelocationType : uint8_t {
  RelocInternal = 0,
  RelocOrdinal = 1,
  RelocName = 2,
  RelocOSFixup = 3,
  RelocAdditive = 4,
};

struct Header {
  support::ulittle16_t Signature;
  uint8_t LinkerVersion;
  uint8_t LinkerRevision;
  support::ulittle16_t EntryTableOffset;
  support::ulittle16_t EntryTableSize;
  support::ulittle32_t Checksum;
  support::ulittle16_t Flags;
  support::ulittle16_t AutoDataSegment;
  support::ulittle16_t InitialHeapSize;
  support::ulittle16_t InitialStackSize;
  support::ulittle32_t EntryPoint;
  support::ulittle32_t InitialStack;
  support::ulittle16_t SegmentCount;
  support::ulittle16_t ModuleReferenceCount;
  support::ulittle16_t NonResidentNameTableSize;
  support::ulittle16_t SegmentTableOffset;
  support::ulittle16_t ResourceTableOffset;
  support::ulittle16_t ResidentNameTableOffset;
  support::ulittle16_t ModuleReferenceTableOffset;
  support::ulittle16_t ImportedNameTableOffset;
  support::ulittle32_t NonResidentNameTableOffset;
  support::ulittle16_t MovableEntryCount;
  support::ulittle16_t SegmentAlignmentShift;
  support::ulittle16_t ResourceSegmentCount;
  uint8_t TargetOS;
  uint8_t OtherFlags;
  support::ulittle16_t ReturnThunkOffset;
  support::ulittle16_t SegmentReferenceBytesOffset;
  support::ulittle16_t MinimumCodeSwapArea;
  support::ulittle16_t ExpectedWindowsVersion;
};

static_assert(sizeof(Header) == 0x40, "invalid NE header size");

struct Segment {
  support::ulittle16_t DataOffset;
  support::ulittle16_t DataLength;
  support::ulittle16_t Flags;
  support::ulittle16_t MinimumAllocation;
};

static_assert(sizeof(Segment) == 8, "invalid NE segment entry size");

struct Relocation {
  uint8_t AddressType;
  uint8_t Type;
  support::ulittle16_t Offset;
  support::ulittle16_t Target1;
  support::ulittle16_t Target2;
};

static_assert(sizeof(Relocation) == 8, "invalid NE relocation entry size");

} // namespace NE
} // namespace llvm

#endif // LLVM_BINARYFORMAT_NE_H
