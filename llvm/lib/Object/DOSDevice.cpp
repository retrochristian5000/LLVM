//===- DOSDevice.cpp - Raw DOS device-header reader -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Object/DOSDevice.h"
#include "llvm/Object/Error.h"

using namespace llvm;
using namespace llvm::object;

Expected<DOSDevice::Header>
DOSDevice::parseHeader(MemoryBufferRef Source) {
  StringRef Data = Source.getBuffer();
  auto invalid = [&](const char *Reason) -> Error {
    return make_error<GenericBinaryError>(
        Source.getBufferIdentifier() + ": " + Reason,
        object_error::parse_failed);
  };
  constexpr size_t HeaderSize = 18;
  if (Data.size() < HeaderSize)
    return invalid("truncated DOS device header");

  // Decode each field from unaligned bytes; never use host structure layout.
  auto byteAt = [&](size_t Offset) -> uint8_t {
    return static_cast<uint8_t>(Data[Offset]);
  };
  auto wordAt = [&](size_t Offset) -> uint16_t {
    return uint16_t(byteAt(Offset)) | (uint16_t(byteAt(Offset + 1)) << 8);
  };

  Header H;
  H.NextDevice = uint32_t(wordAt(0)) | (uint32_t(wordAt(2)) << 16);
  H.Attributes = wordAt(4);
  H.StrategyOffset = wordAt(6);
  H.InterruptOffset = wordAt(8);
  for (size_t I = 0; I != H.NameOrUnits.size(); ++I)
    H.NameOrUnits[I] = Data[10 + I];

  // Microsoft DOS 3.3 allows a chain of headers in one image: the low word
  // of the initial pointer then names an in-image header offset. Do not
  // follow or validate the rest of that chain here.
  if (H.NextDevice != 0xffffffffu) {
    size_t NextOffset = H.NextDevice & 0xffffu;
    if (NextOffset < HeaderSize || NextOffset > Data.size() - HeaderSize)
      return invalid("invalid initial DOS device header link");
  }

  // The strategy/interrupt offsets are near offsets within the same load
  // segment as the device header, not host virtual addresses.
  if (H.StrategyOffset < HeaderSize || H.StrategyOffset >= Data.size() ||
      H.InterruptOffset < HeaderSize || H.InterruptOffset >= Data.size())
    return invalid("DOS device entry point exceeds image bounds");

  return H;
}
