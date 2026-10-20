//===-- ClangExpressionDeclMapHeaderTest.cpp ------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Intentionally include the declaration-map header first. In particular,
// do not include system headers such as <signal.h> as a workaround for
// missing size_t declarations in the LLDB headers. This translation unit
// must compile with the selected target SDK and C++ standard.
#include "Plugins/ExpressionParser/Clang/ClangExpressionDeclMap.h"
#include "Plugins/ExpressionParser/Clang/ClangExpressionVariable.h"

#include <memory>
#include <type_traits>

using lldb_private::ClangExpressionDeclMap;
using lldb_private::ClangExpressionVariable;

static_assert(sizeof(ClangExpressionDeclMap) > 0);
static_assert(sizeof(ClangExpressionVariable) > 0);
static_assert(std::is_destructible_v<ClangExpressionDeclMap>);
static_assert(std::is_destructible_v<ClangExpressionVariable>);
static_assert(std::is_same_v<decltype(std::make_unique<ClangExpressionDeclMap>()),
                             std::unique_ptr<ClangExpressionDeclMap>> == false,
              "DeclMap must not be implicitly default constructible");
