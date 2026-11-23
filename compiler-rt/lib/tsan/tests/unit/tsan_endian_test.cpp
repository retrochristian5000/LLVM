//===-- tsan_endian_test.cpp ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tsan_endian.h"
#include "gtest/gtest.h"

namespace __tsan {

TEST(Endian, LittleEndian32WordsUnaligned) {
  // The first byte deliberately misaligns the first input word.
  const u8 bytes[] = {0xff, 0x78, 0x56, 0x34, 0x12,
                     0xef, 0xcd, 0xab, 0x90};
  u32 words[2] = {};
  ReadLittleEndian32Words(words, bytes + 1, 2);
  EXPECT_EQ(0x12345678u, words[0]);
  EXPECT_EQ(0x90abcdefu, words[1]);
}

static void CheckMD5(const char *text, uptr length,
                     const char *expected_hex) {
  const MD5Hash result = md5_hash(text, length);
  const u8 *digest = reinterpret_cast<const u8 *>(result.hash);
  const char digits[] = "0123456789abcdef";
  for (uptr i = 0; i < 16; ++i) {
    EXPECT_EQ(expected_hex[2 * i], digits[digest[i] >> 4]);
    EXPECT_EQ(expected_hex[2 * i + 1], digits[digest[i] & 15]);
  }
}

TEST(Endian, MD5ReferenceVectors) {
  CheckMD5("", 0, "d41d8cd98f00b204e9800998ecf8427e");
  CheckMD5("abc", 3, "900150983cd24fb0d6963f7d28e17f72");

  // Exercise the direct full-block path and the final padded block.
  char input[65];
  for (char &c : input)
    c = 'a';
  CheckMD5(input, 64, "014842d480b571495a4a0363793f7367");
  CheckMD5(input, 65, "c743a45e0d2e6a95cb859adae0248435");
}

}  // namespace __tsan
