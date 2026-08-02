//===- unittests/Sema/APINotesSelectorTest.cpp ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Sema/APINotesSelector.h"
#include "clang/APINotes/Types.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/ASTUnit.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "gtest/gtest.h"
#include <initializer_list>
#include <string>
#include <vector>

using namespace clang;

namespace {

using clang::ast_matchers::functionDecl;
using clang::ast_matchers::hasName;
using clang::ast_matchers::match;
using clang::ast_matchers::unless;
using clang::tooling::buildASTFromCodeWithArgs;

llvm::SmallVector<std::string, 4>
makeParameterList(std::initializer_list<llvm::StringRef> Parameters) {
  llvm::SmallVector<std::string, 4> Result;
  for (llvm::StringRef Parameter : Parameters)
    Result.push_back(Parameter.str());
  return Result;
}

std::string formatSelector(llvm::ArrayRef<std::string> Parameters) {
  return api_notes::formatAPINotesParameterSelector(Parameters);
}

void expectParameterList(llvm::ArrayRef<std::string> Actual,
                         std::initializer_list<llvm::StringRef> ExpectedRefs,
                         llvm::StringRef Label) {
  llvm::SmallVector<std::string, 4> Expected = makeParameterList(ExpectedRefs);

  EXPECT_EQ(Actual.size(), Expected.size())
      << Label << " selector: expected " << formatSelector(Expected) << ", got "
      << formatSelector(Actual);
  if (Actual.size() != Expected.size())
    return;

  for (unsigned I = 0, E = Expected.size(); I != E; ++I) {
    EXPECT_EQ(Actual[I], Expected[I])
        << Label << " selector: expected " << formatSelector(Expected)
        << ", got " << formatSelector(Actual);
  }
}

const FunctionDecl *findTarget(ASTUnit &AST) {
  auto Results =
      match(functionDecl(hasName("target"), unless(ast_matchers::isImplicit()))
                .bind("fn"),
            AST.getASTContext());
  EXPECT_EQ(Results.size(), 1u);
  if (Results.size() != 1u)
    return nullptr;
  return Results[0].getNodeAs<FunctionDecl>("fn");
}

void expectSelectors(llvm::StringRef Code,
                     std::initializer_list<llvm::StringRef> Source,
                     std::initializer_list<llvm::StringRef> Desugared = {},
                     bool ExpectDesugared = false,
                     bool IsObjectiveCXX = false) {
  std::vector<std::string> Args;
  std::string FileName;
  if (IsObjectiveCXX) {
    Args = {"-x", "objective-c++", "-std=c++20"};
    FileName = "input.mm";
  } else {
    Args = {"-std=c++20"};
    FileName = "input.cpp";
  }

  std::unique_ptr<ASTUnit> AST = buildASTFromCodeWithArgs(Code, Args, FileName);
  ASSERT_TRUE(AST);

  const FunctionDecl *Target = findTarget(*AST);
  ASSERT_NE(Target, nullptr);

  std::optional<APINotesParameterSelectorCandidates> Candidates =
      getAPINotesParameterSelectorCandidates(AST->getASTContext(), Target);
  ASSERT_TRUE(Candidates);

  expectParameterList(Candidates->Source.Parameters, Source, "source");

  EXPECT_EQ(Candidates->Desugared.has_value(), ExpectDesugared);
  if (ExpectDesugared)
    expectParameterList(Candidates->Desugared->Parameters, Desugared,
                        "desugared");
}

TEST(APINotesSelectorTest, ExtractsZeroParameterSelector) {
  expectSelectors("void target();", {});
}

TEST(APINotesSelectorTest, ExtractsMultipleParametersAndIgnoresDefaults) {
  expectSelectors("void target(int, double = 0);", {"int", "double"});
}

TEST(APINotesSelectorTest, DropsTopLevelConstFromValueParameter) {
  expectSelectors("void target(const int);", {"int"});
}

TEST(APINotesSelectorTest, NormalizesPointerAndReferenceSpacing) {
  expectSelectors("void target(int *, int &, int &&);",
                  {"int*", "int&", "int&&"});
}

TEST(APINotesSelectorTest, DropsTopLevelConstFromPointerValueParameter) {
  expectSelectors("void target(int *const);", {"int*"});
}

TEST(APINotesSelectorTest, PreservesPointeeConstOnPointerParameter) {
  expectSelectors("void target(const int *);", {"const int*"});
}

TEST(APINotesSelectorTest, NormalizesTemplateSpacing) {
  expectSelectors(R"cpp(
    template <typename T, typename U> struct Box {};
    void target(Box<int, double>);
  )cpp",
                  {"Box<int,double>"});
}

TEST(APINotesSelectorTest,
     PreservesAliasAsSourceSelectorWithDesugaredFallback) {
  expectSelectors(R"cpp(
    using AliasInt = int;
    void target(AliasInt);
  )cpp",
                  {"AliasInt"}, {"int"}, /*ExpectDesugared=*/true);
}

TEST(APINotesSelectorTest,
     PreservesDeepAliasAsSourceSelectorWithDesugaredFallback) {
  expectSelectors(R"cpp(
    using AliasInt = int;
    using DeepAliasInt = AliasInt;
    void target(DeepAliasInt);
  )cpp",
                  {"DeepAliasInt"}, {"int"}, /*ExpectDesugared=*/true);
}

TEST(APINotesSelectorTest, StripsParameterNullability) {
  expectSelectors("void target(char * _Nonnull);", {"char*"},
                  /*Desugared=*/{}, /*ExpectDesugared=*/false,
                  /*IsObjectiveCXX=*/true);
}

} // namespace
