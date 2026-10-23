//===- DOSDevice.h - Raw DOS installable device-driver header -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_OBJECT_DOSDEVICE_H
#define LLVM_OBJECT_DOSDEVICE_H

#include "llvm/Support/Compiler.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/MemoryBuffer.h"
#include <array>
#include <cstdint>

namespace llvm {
namespace object {
namespace DOSDevice {

// First 18 bytes of a headerless real-mode DOS CONFIG.SYS DEVICE= image.
// The link is file-initialized to FFFF:FFFF for a single device; chained
// drivers may instead provide the offset of another in-image header.
// These offsets use the driver's load segment, not MZ file offsets.
struct Header {
  uint32_t NextDevice = 0;
  uint16_t Attributes = 0;
  uint16_t StrategyOffset = 0;
  uint16_t InterruptOffset = 0;
  std::array<char, 8> NameOrUnits{};

  bool isCharacterDevice() const { return (Attributes & 0x8000) != 0; }
};

// Explicit parser: no magic-byte or .SYS extension inference. This reads
// the first header only; it does not load or execute the driver, traverse
// header chains, or parse MZ-format device drivers.
LLVM_ABI Expected<Header> parseHeader(MemoryBufferRef Source);

} // namespace DOSDevice
} // namespace object
} // namespace llvm

#endif // LLVM_OBJECT_DOSDEVICE_H
