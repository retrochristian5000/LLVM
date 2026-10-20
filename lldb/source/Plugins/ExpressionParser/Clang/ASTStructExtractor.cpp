//===-- ASTStructExtractor.cpp --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ASTStructExtractor.h"
#include "ClangFunctionCaller.h"

#include "lldb/Utility/Log.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/Stmt.h"
#include "clang/Parse/Parser.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdlib>
#include <utility>

using namespace llvm;
using namespace clang;
using namespace lldb_private;

std::optional<ASTStructExtractor::WrapperLayout>
ASTStructExtractor::ValidateWrapperLayout(
    uint64_t struct_size, uint64_t data_size,
    ArrayRef<uint64_t> field_offset_bits, size_t argument_count,
    uint64_t char_width) {
  // The first member is a function pointer and the last one is the result.
  // Never evaluate getFieldOffset(field_count - 1) for an empty record.
  if (field_offset_bits.size() < 2 ||
      argument_count != field_offset_bits.size() - 2 ||
      field_offset_bits.front() != 0 || char_width == 0 ||
      data_size == 0 || data_size > struct_size)
    return std::nullopt;

  WrapperLayout result{struct_size, 0, 0, {}};
  result.member_offsets.reserve(field_offset_bits.size());

  for (uint64_t bit_offset : field_offset_bits) {
    if (bit_offset % char_width)
      return std::nullopt;
    uint64_t byte_offset = bit_offset / char_width;
    // The synthesized wrapper contains ordinary, non-overlapping members.
    if (byte_offset >= data_size ||
        (!result.member_offsets.empty() &&
         byte_offset <= result.member_offsets.back()))
      return std::nullopt;
    result.member_offsets.push_back(byte_offset);
  }

  result.return_offset = result.member_offsets.back();
  result.return_size = data_size - result.return_offset;
  return result;
}

ASTStructExtractor::ASTStructExtractor(ASTConsumer *passthrough,
                                       const char *struct_name,
                                       ClangFunctionCaller &function)
    : m_ast_context(nullptr), m_passthrough(passthrough),
      m_passthrough_sema(nullptr), m_sema(nullptr), m_function(function),
      m_struct_name(struct_name) {
  if (!m_passthrough)
    return;

  m_passthrough_sema = dyn_cast<SemaConsumer>(passthrough);
}

ASTStructExtractor::~ASTStructExtractor() = default;

void ASTStructExtractor::Initialize(ASTContext &Context) {
  m_ast_context = &Context;

  if (m_passthrough)
    m_passthrough->Initialize(Context);
}

void ASTStructExtractor::ExtractFromFunctionDecl(FunctionDecl *F) {
  if (!F->hasBody())
    return;

  Stmt *body_stmt = F->getBody();
  CompoundStmt *body_compound_stmt = dyn_cast<CompoundStmt>(body_stmt);

  if (!body_compound_stmt)
    return; // do we have to handle this?

  RecordDecl *struct_decl = nullptr;

  StringRef desired_name(m_struct_name);

  for (CompoundStmt::const_body_iterator bi = body_compound_stmt->body_begin(),
                                         be = body_compound_stmt->body_end();
       bi != be; ++bi) {
    Stmt *curr_stmt = *bi;
    DeclStmt *curr_decl_stmt = dyn_cast<DeclStmt>(curr_stmt);
    if (!curr_decl_stmt)
      continue;
    DeclGroupRef decl_group = curr_decl_stmt->getDeclGroup();
    for (Decl *candidate_decl : decl_group) {
      RecordDecl *candidate_record_decl = dyn_cast<RecordDecl>(candidate_decl);
      if (!candidate_record_decl)
        continue;
      if (candidate_record_decl->getName() == desired_name) {
        struct_decl = candidate_record_decl;
        break;
      }
    }
    if (struct_decl)
      break;
  }

  if (!struct_decl)
    return;

  // A declaration may still be incomplete or invalid when AST callbacks
  // arrive. Do not ask Clang for its record layout until it is complete.
  if (!struct_decl->isCompleteDefinition() || struct_decl->isInvalidDecl() ||
      !struct_decl->isStruct())
    return;

  const ASTRecordLayout &struct_layout =
      m_ast_context->getASTRecordLayout(struct_decl);

  std::vector<uint64_t> field_offsets;
  field_offsets.reserve(struct_layout.getFieldCount());
  for (const FieldDecl *field : struct_decl->fields()) {
    // The generated wrapper must not contain bitfields.
    if (field->isBitField())
      return;
  }
  for (unsigned i = 0; i < struct_layout.getFieldCount(); ++i)
    field_offsets.push_back(struct_layout.getFieldOffset(i));

  auto layout = ValidateWrapperLayout(
      struct_layout.getSize().getQuantity(),
      struct_layout.getDataSize().getQuantity(), field_offsets,
      m_function.m_arg_values.GetSize(), m_ast_context->getCharWidth());
  if (!layout)
    return;

  // Publish only after validating the whole layout; a rejected declaration
  // must not leave stale or partially populated member offsets.
  m_function.m_struct_size = layout->struct_size;
  m_function.m_return_offset = layout->return_offset;
  m_function.m_return_size = layout->return_size;
  m_function.m_member_offsets = std::move(layout->member_offsets);
  m_function.m_struct_valid = true;
}

void ASTStructExtractor::ExtractFromTopLevelDecl(Decl *D) {
  LinkageSpecDecl *linkage_spec_decl = dyn_cast<LinkageSpecDecl>(D);

  if (linkage_spec_decl) {
    RecordDecl::decl_iterator decl_iterator;

    for (decl_iterator = linkage_spec_decl->decls_begin();
         decl_iterator != linkage_spec_decl->decls_end(); ++decl_iterator) {
      ExtractFromTopLevelDecl(*decl_iterator);
    }
  }

  FunctionDecl *function_decl = dyn_cast<FunctionDecl>(D);

  if (m_ast_context && function_decl &&
      m_function.m_wrapper_function_name == function_decl->getNameAsString()) {
    ExtractFromFunctionDecl(function_decl);
  }
}

bool ASTStructExtractor::HandleTopLevelDecl(DeclGroupRef D) {
  DeclGroupRef::iterator decl_iterator;

  for (decl_iterator = D.begin(); decl_iterator != D.end(); ++decl_iterator) {
    Decl *decl = *decl_iterator;

    ExtractFromTopLevelDecl(decl);
  }

  if (m_passthrough)
    return m_passthrough->HandleTopLevelDecl(D);
  return true;
}

// The wrapper is an ASTConsumer in front of CodeGen. When Clang calls a
// callback other than HandleTopLevelDecl, preserve the downstream consumer's
// notification rather than silently accepting AST events on its behalf.
void ASTStructExtractor::HandleInlineFunctionDefinition(FunctionDecl *D) {
  if (m_passthrough)
    m_passthrough->HandleInlineFunctionDefinition(D);
}

void ASTStructExtractor::HandleInterestingDecl(DeclGroupRef D) {
  for (Decl *decl : D)
    ExtractFromTopLevelDecl(decl);
  if (m_passthrough)
    m_passthrough->HandleInterestingDecl(D);
}

void ASTStructExtractor::HandleTagDeclRequiredDefinition(const TagDecl *D) {
  if (m_passthrough)
    m_passthrough->HandleTagDeclRequiredDefinition(D);
}

void ASTStructExtractor::HandleCXXImplicitFunctionInstantiation(FunctionDecl *D) {
  if (m_passthrough)
    m_passthrough->HandleCXXImplicitFunctionInstantiation(D);
}

void ASTStructExtractor::HandleTopLevelDeclInObjCContainer(DeclGroupRef D) {
  if (m_passthrough)
    m_passthrough->HandleTopLevelDeclInObjCContainer(D);
}

void ASTStructExtractor::HandleImplicitImportDecl(ImportDecl *D) {
  if (m_passthrough)
    m_passthrough->HandleImplicitImportDecl(D);
}

void ASTStructExtractor::CompleteExternalDeclaration(DeclaratorDecl *D) {
  if (m_passthrough)
    m_passthrough->CompleteExternalDeclaration(D);
}

void ASTStructExtractor::AssignInheritanceModel(CXXRecordDecl *RD) {
  if (m_passthrough)
    m_passthrough->AssignInheritanceModel(RD);
}

void ASTStructExtractor::HandleCXXStaticMemberVarInstantiation(VarDecl *D) {
  if (m_passthrough)
    m_passthrough->HandleCXXStaticMemberVarInstantiation(D);
}

void ASTStructExtractor::HandleOpenACCRoutineReference(
    const FunctionDecl *FD, const OpenACCRoutineDecl *RD) {
  if (m_passthrough)
    m_passthrough->HandleOpenACCRoutineReference(FD, RD);
}

ASTMutationListener *ASTStructExtractor::GetASTMutationListener() {
  return m_passthrough ? m_passthrough->GetASTMutationListener() : nullptr;
}

ASTDeserializationListener *ASTStructExtractor::GetASTDeserializationListener() {
  return m_passthrough ? m_passthrough->GetASTDeserializationListener() : nullptr;
}

bool ASTStructExtractor::shouldSkipFunctionBody(Decl *D) {
  // The wrapper's body contains the only available argument struct layout.
  // It must be parsed even when the underlying consumer permits body skipping.
  if (auto *function = dyn_cast_or_null<FunctionDecl>(D))
    if (function->getName() == StringRef(m_function.m_wrapper_function_name))
      return false;

  return m_passthrough ? m_passthrough->shouldSkipFunctionBody(D)
                       : SemaConsumer::shouldSkipFunctionBody(D);
}

void ASTStructExtractor::HandleTranslationUnit(ASTContext &Ctx) {
  if (m_passthrough)
    m_passthrough->HandleTranslationUnit(Ctx);
}

void ASTStructExtractor::HandleTagDeclDefinition(TagDecl *D) {
  if (m_passthrough)
    m_passthrough->HandleTagDeclDefinition(D);
}

void ASTStructExtractor::CompleteTentativeDefinition(VarDecl *D) {
  if (m_passthrough)
    m_passthrough->CompleteTentativeDefinition(D);
}

void ASTStructExtractor::HandleVTable(CXXRecordDecl *RD) {
  if (m_passthrough)
    m_passthrough->HandleVTable(RD);
}

void ASTStructExtractor::PrintStats() {
  if (m_passthrough)
    m_passthrough->PrintStats();
}

void ASTStructExtractor::InitializeSema(Sema &S) {
  m_sema = &S;

  if (m_passthrough_sema)
    m_passthrough_sema->InitializeSema(S);
}

void ASTStructExtractor::ForgetSema() {
  m_sema = nullptr;

  if (m_passthrough_sema)
    m_passthrough_sema->ForgetSema();
}
