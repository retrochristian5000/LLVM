//===-- ASTStructExtractorTest.cpp ----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Plugins/ExpressionParser/Clang/ASTStructExtractor.h"
#include "gtest/gtest.h"

#include <array>
#include <cstdint>
#include <type_traits>

using lldb_private::ASTStructExtractor;

// A standalone include of ASTStructExtractor.h must expose the Clang callback
// signatures without importing the implementation-only function-caller header.
static_assert(std::is_base_of_v<clang::SemaConsumer, ASTStructExtractor>);
static_assert(std::is_same_v<
              decltype(&ASTStructExtractor::HandleInlineFunctionDefinition),
              void (ASTStructExtractor::*)(clang::FunctionDecl *)>);
static_assert(std::is_same_v<
              decltype(&ASTStructExtractor::HandleTagDeclRequiredDefinition),
              void (ASTStructExtractor::*)(const clang::TagDecl *)>);
static_assert(std::is_same_v<
              decltype(&ASTStructExtractor::shouldSkipFunctionBody),
              bool (ASTStructExtractor::*)(clang::Decl *)>);

TEST(ASTStructExtractorTest, ValidWrapperLayout) {
  // A pointer, one argument, then a four-byte return slot.
  constexpr std::array<uint64_t, 3> offsets = {0, 64, 128};
  auto layout = ASTStructExtractor::ValidateWrapperLayout(
      /*struct_size=*/24, /*data_size=*/20, offsets,
      /*argument_count=*/1, /*char_width=*/8);
  ASSERT_TRUE(layout);
  EXPECT_EQ(layout->struct_size, 24u);
  EXPECT_EQ(layout->return_offset, 16u);
  EXPECT_EQ(layout->return_size, 4u);
  EXPECT_EQ(layout->member_offsets,
            (std::vector<uint64_t>{0, 8, 16}));
}

TEST(ASTStructExtractorTest, ZeroArgumentWrapper) {
  constexpr std::array<uint64_t, 2> offsets = {0, 32};
  auto layout = ASTStructExtractor::ValidateWrapperLayout(
      8, 8, offsets, 0, 8);
  ASSERT_TRUE(layout);
  EXPECT_EQ(layout->member_offsets, (std::vector<uint64_t>{0, 4}));
  EXPECT_EQ(layout->return_offset, 4u);
}

TEST(ASTStructExtractorTest, TargetCharWidth) {
  // Offsets use target bits, not hardcoded divisions by 8.
  constexpr std::array<uint64_t, 2> offsets = {0, 64};
  auto layout = ASTStructExtractor::ValidateWrapperLayout(
      8, 8, offsets, 0, 16);
  ASSERT_TRUE(layout);
  EXPECT_EQ(layout->return_offset, 4u);
}

TEST(ASTStructExtractorTest, RejectMalformedRecordLayouts) {
  constexpr std::array<uint64_t, 0> empty = {};
  constexpr std::array<uint64_t, 1> one_field = {0};
  constexpr std::array<uint64_t, 3> fields = {0, 64, 128};
  constexpr std::array<uint64_t, 3> unaligned = {0, 65, 128};
  constexpr std::array<uint64_t, 3> overlapping = {0, 64, 64};
  constexpr std::array<uint64_t, 3> wrong_first_offset = {8, 64, 128};

  EXPECT_FALSE(ASTStructExtractor::ValidateWrapperLayout(24, 20, empty, 0, 8));
  EXPECT_FALSE(
      ASTStructExtractor::ValidateWrapperLayout(24, 20, one_field, 0, 8));
  EXPECT_FALSE(ASTStructExtractor::ValidateWrapperLayout(24, 20, fields, 0, 8));
  EXPECT_FALSE(
      ASTStructExtractor::ValidateWrapperLayout(24, 20, unaligned, 1, 8));
  EXPECT_FALSE(
      ASTStructExtractor::ValidateWrapperLayout(24, 20, overlapping, 1, 8));
  EXPECT_FALSE(ASTStructExtractor::ValidateWrapperLayout(24, 20, fields, 1, 0));
  EXPECT_FALSE(ASTStructExtractor::ValidateWrapperLayout(24, 16, fields, 1, 8));
  EXPECT_FALSE(ASTStructExtractor::ValidateWrapperLayout(20, 24, fields, 1, 8));
  EXPECT_FALSE(ASTStructExtractor::ValidateWrapperLayout(
      24, 20, wrong_first_offset, 1, 8));
}
