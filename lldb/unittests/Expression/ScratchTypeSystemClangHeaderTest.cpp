//===-- ScratchTypeSystemClangHeaderTest.cpp -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Intentionally include the LLDB type-system header first: consumers must
// be able to destroy a ScratchTypeSystemClang without including the private
// ClangPersistentVariables or ClangASTSource definitions. The previous
// inline defaulted destructor could instantiate libc++ default_delete<T>'s
// sizeof(T) assertion on these forward-declared types.
#include "Plugins/TypeSystem/Clang/TypeSystemClang.h"

#include "gtest/gtest.h"
#include <memory>
#include <type_traits>

using lldb_private::ScratchTypeSystemClang;

static_assert(sizeof(ScratchTypeSystemClang) > 0);
static_assert(std::is_destructible_v<ScratchTypeSystemClang>);

TEST(ScratchTypeSystemClangHeaderTest, OutOfLineDestructor) {
  // Compiles both direct deletion and std::unique_ptr's deleter with only
  // TypeSystemClang.h visible. No actual Target/AST construction is needed.
  ScratchTypeSystemClang *raw = nullptr;
  delete raw;
  std::unique_ptr<ScratchTypeSystemClang> ptr;
  EXPECT_EQ(ptr.get(), nullptr);
}
