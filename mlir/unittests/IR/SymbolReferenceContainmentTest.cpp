//===- SymbolReferenceContainmentTest.cpp - Containment bit unit tests ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Tests for the interning-time bit that records whether a type or attribute
// transitively contains a SymbolRefAttr (conservatively true for mutable
// storage). Symbol-table verification and the symbol-use walks rely on it to
// skip types and attributes that provably hold no symbol reference. Because a
// SymbolUserTypeInterface / SymbolUserAttrInterface implementation must spell
// its references as SymbolRefAttr sub-elements, the bit being clear is a sound
// reason to skip an instance even after the interface is attached late.
//
//===----------------------------------------------------------------------===//

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/OwningOpRef.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "gtest/gtest.h"

#include "../../test/lib/Dialect/Test/TestAttributes.h"
#include "../../test/lib/Dialect/Test/TestDialect.h"
#include "../../test/lib/Dialect/Test/TestTypes.h"

using namespace mlir;

namespace {

// Symbol-user models whose verification always fails, attached externally to
// exercise late interface attachment. One targets a type that structurally
// holds a SymbolRefAttr (a tensor with a symbol-ref encoding); the other
// targets f32, which holds none.
struct FailingTensorSymbolUserModel
    : public SymbolUserTypeInterface::ExternalModel<
          FailingTensorSymbolUserModel, RankedTensorType> {
  LogicalResult verifySymbolUses(Type type, Operation *op,
                                 SymbolTableCollection &symbolTable) const {
    return op->emitError("tensor rejected by its attached symbol-user model");
  }
};
struct FailingF32SymbolUserModel
    : public SymbolUserTypeInterface::ExternalModel<FailingF32SymbolUserModel,
                                                    Float32Type> {
  LogicalResult verifySymbolUses(Type type, Operation *op,
                                 SymbolTableCollection &symbolTable) const {
    return op->emitError("f32 rejected by its attached symbol-user model");
  }
};

class SymbolReferenceContainmentTest : public ::testing::Test {
protected:
  SymbolReferenceContainmentTest() {
    context.loadDialect<test::TestDialect>();
    context.allowUnregisteredDialects();
  }

  FlatSymbolRefAttr symbolRef() {
    return FlatSymbolRefAttr::get(&context, "sym");
  }

  // A conforming type implementing SymbolUserTypeInterface, spelling its
  // reference as a FlatSymbolRefAttr parameter: !test.symbol_ref<@sym>.
  test::TestSymbolUserType symbolUserType() {
    return test::TestSymbolUserType::get(&context, symbolRef());
  }

  // A conforming attribute implementing SymbolUserAttrInterface, spelling its
  // reference as a FlatSymbolRefAttr parameter: #test.symbol_ref_attr<@sym>.
  test::TestSymbolRefAttr symbolUserAttr() {
    return test::TestSymbolRefAttr::get(&context, symbolRef());
  }

  MLIRContext context;
};

// A leaf type holding no symbol reference carries no bit.
TEST_F(SymbolReferenceContainmentTest, LeafTypeIsClear) {
  EXPECT_FALSE(IntegerType::get(&context, 32).mayContainSymbolRefs());
}

// A plain attribute holding no symbol reference carries no bit.
TEST_F(SymbolReferenceContainmentTest, LeafAttrIsClear) {
  EXPECT_FALSE(StringAttr::get(&context, "hi").mayContainSymbolRefs());
  EXPECT_FALSE(
      TypeAttr::get(IntegerType::get(&context, 32)).mayContainSymbolRefs());
}

// A SymbolRefAttr itself carries the bit.
TEST_F(SymbolReferenceContainmentTest, FlatSymbolRefAttrHasBit) {
  EXPECT_TRUE(symbolRef().mayContainSymbolRefs());
}

// A non-flat SymbolRefAttr, which nests further references, carries the bit.
TEST_F(SymbolReferenceContainmentTest, NestedSymbolRefAttrHasBit) {
  SymbolRefAttr ref =
      SymbolRefAttr::get(StringAttr::get(&context, "root"),
                         {FlatSymbolRefAttr::get(&context, "n")});
  EXPECT_TRUE(ref.mayContainSymbolRefs());
}

// A conforming symbol-user type carries the bit through its SymbolRefAttr
// parameter (not through the interface, which plays no part in the bit).
TEST_F(SymbolReferenceContainmentTest, ConformingSymbolUserTypeHasBit) {
  EXPECT_TRUE(symbolUserType().mayContainSymbolRefs());
}

// A conforming symbol-user attribute carries the bit through its SymbolRefAttr
// parameter.
TEST_F(SymbolReferenceContainmentTest, ConformingSymbolUserAttrHasBit) {
  EXPECT_TRUE(symbolUserAttr().mayContainSymbolRefs());
}

// A type nesting a symbol-ref-bearing type propagates the bit.
TEST_F(SymbolReferenceContainmentTest, TypeNestingSymbolRefBearingType) {
  EXPECT_TRUE(
      TupleType::get(&context, {symbolUserType()}).mayContainSymbolRefs());
}

// A tuple of ordinary types stays clear.
TEST_F(SymbolReferenceContainmentTest, TypeNestingOrdinaryTypesIsClear) {
  Type i32 = IntegerType::get(&context, 32);
  EXPECT_FALSE(TupleType::get(&context, {i32, i32}).mayContainSymbolRefs());
}

// A type reaches a SymbolRefAttr two levels deep, through an attribute
// sub-element (a tensor encoding holding a TypeAttr of a symbol-ref type).
TEST_F(SymbolReferenceContainmentTest, TypeReachesSymbolRefThroughAttribute) {
  Attribute encoding = TypeAttr::get(symbolUserType());
  EXPECT_TRUE(encoding.mayContainSymbolRefs());
  RankedTensorType tensor =
      RankedTensorType::get({2}, IntegerType::get(&context, 32), encoding);
  EXPECT_TRUE(tensor.mayContainSymbolRefs());
}

// A type reaches a plain SymbolRefAttr through an attribute parameter (a tensor
// encoding).
TEST_F(SymbolReferenceContainmentTest, TypeWithSymbolRefAttrParameter) {
  RankedTensorType tensor =
      RankedTensorType::get({2}, IntegerType::get(&context, 32), symbolRef());
  EXPECT_TRUE(tensor.mayContainSymbolRefs());
}

// A dictionary attribute containing a plain SymbolRefAttr carries the bit.
TEST_F(SymbolReferenceContainmentTest, DictionaryAttrContainingSymbolRef) {
  NamedAttribute named(StringAttr::get(&context, "callee"), symbolRef());
  EXPECT_TRUE(DictionaryAttr::get(&context, {named}).mayContainSymbolRefs());
}

// A dictionary attribute containing a symbol-ref-bearing type inside a TypeAttr
// carries the bit; the bit on the dictionary summarizes its whole nested tree.
TEST_F(SymbolReferenceContainmentTest, DictionaryAttrContainingSymbolRefType) {
  NamedAttribute named(StringAttr::get(&context, "key"),
                       TypeAttr::get(symbolUserType()));
  EXPECT_TRUE(DictionaryAttr::get(&context, {named}).mayContainSymbolRefs());
}

// A dictionary attribute with no symbol reference stays clear.
TEST_F(SymbolReferenceContainmentTest, DictionaryAttrIsClear) {
  NamedAttribute named(StringAttr::get(&context, "key"),
                       TypeAttr::get(IntegerType::get(&context, 32)));
  EXPECT_FALSE(DictionaryAttr::get(&context, {named}).mayContainSymbolRefs());
}

// A type carrying a mutable component reports the bit conservatively, since its
// sub-elements may change after the bit is fixed at uniquing.
TEST_F(SymbolReferenceContainmentTest, MutableTypeReportsConservatively) {
  test::TestRecursiveType recursive =
      test::TestRecursiveType::get(&context, "rec");
  EXPECT_TRUE(recursive.mayContainSymbolRefs());
}

// Interface membership plays no part in the bit, so late attachment needs no
// fallback: a type that structurally holds a SymbolRefAttr (a tensor with a
// symbol-ref encoding) has its bit set from interning, so verification visits
// it and the newly-attached verifySymbolUses fires.
TEST_F(SymbolReferenceContainmentTest, LateInterfaceAttachmentStillVerifies) {
  OwningOpRef<ModuleOp> module = parseSourceString<ModuleOp>(
      "module { \"foo.op\"() : () -> tensor<4xf32, @sym> }", &context);
  ASSERT_TRUE(module);

  RankedTensorType::attachInterface<FailingTensorSymbolUserModel>(context);
  ScopedDiagnosticHandler handler(&context,
                                  [](Diagnostic &) { return success(); });
  EXPECT_TRUE(failed(verify(*module)));
}

// The contract boundary: a type that references a symbol without spelling it as
// a SymbolRefAttr (here f32, standing in for a non-conforming symbol-user type)
// has a clear bit and is therefore skipped -- its verifySymbolUses never fires,
// so verification succeeds. This is the documented cost of the interface
// contract.
TEST_F(SymbolReferenceContainmentTest, NonConformingSymbolUserTypeIsSkipped) {
  OwningOpRef<ModuleOp> module = parseSourceString<ModuleOp>(
      "module { \"foo.op\"() : () -> f32 }", &context);
  ASSERT_TRUE(module);

  Float32Type::attachInterface<FailingF32SymbolUserModel>(context);
  ScopedDiagnosticHandler handler(&context,
                                  [](Diagnostic &) { return success(); });
  EXPECT_TRUE(succeeded(verify(*module)));
}

} // namespace
