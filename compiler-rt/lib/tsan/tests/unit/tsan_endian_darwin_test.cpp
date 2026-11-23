//===-- tsan_endian_darwin_test.cpp ---------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tsan_endian_darwin.h"
#include "gtest/gtest.h"

#include <mach/machine.h>
#include <stdint.h>

namespace __tsan {

TEST(DarwinEndian, RejectUnsupportedHeaders) {
  mach_header header = {};
  cpu_type_t cpu_type = CPU_TYPE_X86_64;
  cpu_subtype_t subtype = 0;

  EXPECT_EQ(MachOByteOrder::kUnsupported, MachO64ByteOrder(nullptr));
  EXPECT_FALSE(DecodeMachO64Architecture(nullptr, &cpu_type, &subtype));

  header.magic = MH_MAGIC;
  EXPECT_EQ(MachOByteOrder::kUnsupported, MachO64ByteOrder(&header));
  EXPECT_FALSE(DecodeMachO64Architecture(&header, &cpu_type, &subtype));

  header.magic = MH_CIGAM;
  EXPECT_EQ(MachOByteOrder::kUnsupported, MachO64ByteOrder(&header));
  EXPECT_FALSE(DecodeMachO64Architecture(&header, &cpu_type, &subtype));

  header.magic = MH_MAGIC_64;
  EXPECT_FALSE(DecodeMachO64Architecture(&header, nullptr, &subtype));
  EXPECT_FALSE(DecodeMachO64Architecture(&header, &cpu_type, nullptr));

  // Failure must not overwrite an output parameter.
  EXPECT_EQ(CPU_TYPE_X86_64, cpu_type);
}

TEST(DarwinEndian, NativeArchitectureAndSubtypeFlags) {
  mach_header header = {};
  header.magic = MH_MAGIC_64;
  header.cputype = CPU_TYPE_ARM64;
  header.cpusubtype = static_cast<cpu_subtype_t>(
      static_cast<uint32_t>(CPU_SUBTYPE_ARM64E) |
      static_cast<uint32_t>(CPU_SUBTYPE_MASK));

  cpu_type_t cpu_type = 0;
  cpu_subtype_t subtype = 0;
  EXPECT_EQ(MachOByteOrder::kNative, MachO64ByteOrder(&header));
  ASSERT_TRUE(DecodeMachO64Architecture(&header, &cpu_type, &subtype));
  EXPECT_EQ(CPU_TYPE_ARM64, cpu_type);
  EXPECT_EQ(CPU_SUBTYPE_ARM64E, subtype);
}

TEST(DarwinEndian, SwappedArchitectureAndSubtypeFlags) {
  mach_header header = {};
  header.magic = MH_CIGAM_64;
  header.cputype = static_cast<cpu_type_t>(
      __builtin_bswap32(static_cast<uint32_t>(CPU_TYPE_ARM64)));
  header.cpusubtype = static_cast<cpu_subtype_t>(__builtin_bswap32(
      static_cast<uint32_t>(CPU_SUBTYPE_ARM64E) |
      static_cast<uint32_t>(CPU_SUBTYPE_MASK)));

  cpu_type_t cpu_type = 0;
  cpu_subtype_t subtype = 0;
  EXPECT_EQ(MachOByteOrder::kSwapped, MachO64ByteOrder(&header));
  ASSERT_TRUE(DecodeMachO64Architecture(&header, &cpu_type, &subtype));
  EXPECT_EQ(CPU_TYPE_ARM64, cpu_type);
  EXPECT_EQ(CPU_SUBTYPE_ARM64E, subtype);
}

TEST(DarwinEndian, NativeX86Architecture) {
  mach_header header = {};
  header.magic = MH_MAGIC_64;
  header.cputype = CPU_TYPE_X86_64;
  header.cpusubtype = 3;

  cpu_type_t cpu_type = 0;
  cpu_subtype_t subtype = 0;
  ASSERT_TRUE(DecodeMachO64Architecture(&header, &cpu_type, &subtype));
  EXPECT_EQ(CPU_TYPE_X86_64, cpu_type);
  EXPECT_EQ(3, subtype);
}

}  // namespace __tsan
