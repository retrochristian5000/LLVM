//===-- IncompleteTypeTest.cpp - ADT forward-declaration contract ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Exercise the ADT declarations used transitively by LLDB's Clang expression
// parser. A pointer-based container must not ask sizeof/alignof of its
// forward-declared pointee during class instantiation. SmallVector<T> and
// SmallSet<T> that *own* T require a complete T and are intentionally omitted.

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "gtest/gtest.h"

#include <type_traits>

namespace {

struct ForwardNode;
struct OtherNode;
struct NeverCompleted;

// These compile-time checks occur before either pointee is defined.
// Requiring alignof(ForwardNode) or sizeof(ForwardNode) here is a regression.
using ForwardUnion = llvm::PointerUnion<ForwardNode *, OtherNode *>;
static_assert(sizeof(ForwardUnion) == sizeof(void *));
static_assert(sizeof(llvm::DenseMap<ForwardNode *, unsigned>) > 0);
static_assert(sizeof(llvm::SmallPtrSet<ForwardNode *, 4>) > 0);
static_assert(sizeof(llvm::SmallVector<ForwardNode *, 2>) > 0);
static_assert(sizeof(llvm::ArrayRef<ForwardNode>) == 2 * sizeof(void *));
static_assert(llvm::is_incomplete_v<NeverCompleted>);

struct alignas(8) ForwardNode {
  int Value = 0;
};
struct alignas(8) OtherNode {
  int Value = 0;
};

static_assert(!llvm::is_incomplete_v<ForwardNode>);
static_assert(!llvm::is_incomplete_v<OtherNode>);

TEST(IncompleteTypeTest, ForwardDeclaredPointerContainers) {
  ForwardNode First;
  OtherNode Second;

  llvm::DenseMap<ForwardNode *, unsigned> Map;
  Map[&First] = 42;
  EXPECT_EQ(Map.lookup(&First), 42u);

  llvm::SmallVector<ForwardNode *, 2> Values;
  Values.push_back(&First);
  EXPECT_EQ(Values.front(), &First);

  llvm::SmallPtrSet<ForwardNode *, 4> Set;
  EXPECT_TRUE(Set.insert(&First).second);
  EXPECT_TRUE(Set.contains(&First));

  ForwardUnion One(&First);
  ForwardUnion Two(&Second);
  EXPECT_EQ(One.get<ForwardNode *>(), &First);
  EXPECT_EQ(Two.get<OtherNode *>(), &Second);

  llvm::ArrayRef<ForwardNode> Ref(First);
  EXPECT_EQ(Ref.size(), 1u);
  EXPECT_EQ(&Ref.front(), &First);
}

} // namespace
