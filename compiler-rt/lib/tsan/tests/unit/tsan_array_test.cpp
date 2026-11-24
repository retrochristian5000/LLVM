//===-- tsan_array_test.cpp ------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_flags.h"
#include "tsan_rtl.h"
#include "gtest/gtest.h"

#if defined(__aarch64__) && !SANITIZER_GO

namespace __tsan {

extern "C" void __tsan_shadow_set_aarch64(RawShadow *begin, RawShadow *end,
                                           u32 value);

TEST(Array, AArch64ShadowCellsPreserveBoundsAndEmptyWords) {
  constexpr uptr kMaxCells = 65;
  constexpr u32 kCanary = 0xa5a55a5a;
  constexpr u32 kValues[] = {0, 1, 0x87654321, 0xffffffff};

  alignas(16) RawShadow words[(kMaxCells + 2) * 4];

  for (u32 value : kValues) {
    for (uptr cells = 0; cells <= kMaxCells; ++cells) {
      for (uptr i = 0; i < (kMaxCells + 2) * 4; ++i)
        words[i] = static_cast<RawShadow>(kCanary);

      RawShadow *const begin = words + 4;
      __tsan_shadow_set_aarch64(begin, begin + cells * 4, value);

      for (uptr i = 0; i < 4; ++i)
        EXPECT_EQ(kCanary, static_cast<u32>(words[i]));

      for (uptr cell = 0; cell < kMaxCells; ++cell) {
        for (uptr word = 0; word < 4; ++word) {
          const u32 expected =
              cell < cells ? (word == 0 ? value : 0) : kCanary;
          EXPECT_EQ(expected,
                    static_cast<u32>(begin[cell * 4 + word]))
              << "cell=" << cell << ", word=" << word
              << ", initialized cells=" << cells;
        }
      }

      for (uptr i = 0; i < 4; ++i)
        EXPECT_EQ(kCanary, static_cast<u32>(words[(kMaxCells + 1) * 4 + i]));
    }
  }
}

TEST(Array, LargeImitatedWritePreservesMiddleShadow) {
  constexpr uptr kSize = 256 * 1024;
  if (common_flags()->clear_shadow_mmap_threshold >= kSize)
    GTEST_SKIP() << "Shadow remap threshold exceeds the test range";

  void *memory = malloc(kSize);
  ASSERT_NE(memory, nullptr);
  const uptr addr = reinterpret_cast<uptr>(memory);
  ThreadState *thr = cur_thread();
  MemoryRangeImitateWrite(thr, StackTrace::GetCurrentPc(), addr, kSize);
  const RawShadow expected =
      Shadow(thr->fast_state, 0, kShadowCell, kAccessWrite).raw();
  // The old zero-remap shortcut erased the imitated write here.
  for (uptr offset : {uptr(0), kSize / 2, kSize - kShadowCell}) {
    RawShadow *cell = MemToShadow(addr + offset);
    EXPECT_EQ(expected, LoadShadow(cell));
    for (uptr word = 1; word < kShadowCnt; ++word)
      EXPECT_EQ(Shadow::kEmpty, LoadShadow(cell + word));
  }
  free(memory);
}

}  // namespace __tsan

#endif  // __aarch64__ && !SANITIZER_GO
