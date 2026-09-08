//===--- CGCall.cpp - Encapsulate calling convention details --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// These classes wrap the information about a call or function
// definition used to handle ABI compliancy.
//
//===----------------------------------------------------------------------===//

#include "CGCall.h"
#include "ABIInfo.h"
#include "ABIInfoImpl.h"
#include "CGBlocks.h"
#include "CGCXXABI.h"
#include "CGCleanup.h"
#include "CGDebugInfo.h"
#include "CGRecordLayout.h"
#include "CodeGenFunction.h"
#include "CodeGenModule.h"
#include "CodeGenPGO.h"
#include "QualTypeMapper.h"
#include "TargetInfo.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/RecordLayout.h"
#include "clang/Basic/CodeGenOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/CodeGen/CGFunctionInfo.h"
#include "clang/CodeGen/SwiftCallingConv.h"
#include "llvm/ABI/FunctionInfo.h"
#include "llvm/ABI/IRTypeMapper.h"
#include "llvm/ABI/TargetInfo.h"
#include "llvm/ABI/Types.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Assumptions.h"
#include "llvm/IR/AttributeMask.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Type.h"
#include "llvm/Transforms/Utils/Local.h"
#include <optional>
using namespace clang;
using namespace CodeGen;

/***/

unsigned CodeGenTypes::ClangCallConvToLLVMCallConv(CallingConv CC) {
  switch (CC) {
  default:
    return llvm::CallingConv::C;
  case CC_X86StdCall:
    return llvm::CallingConv::X86_StdCall;
  case CC_X86FastCall:
    return llvm::CallingConv::X86_FastCall;
  case CC_X86RegCall:
    return llvm::CallingConv::X86_RegCall;
  case CC_X86ThisCall:
    return llvm::CallingConv::X86_ThisCall;
  case CC_Win64:
    return llvm::CallingConv::Win64;
  case CC_X86_64SysV:
    return llvm::CallingConv::X86_64_SysV;
  case CC_AAPCS:
    return llvm::CallingConv::ARM_AAPCS;
  case CC_AAPCS_VFP:
    return llvm::CallingConv::ARM_AAPCS_VFP;
  case CC_IntelOclBicc:
    return llvm::CallingConv::Intel_OCL_BI;
  case CC_X86Pascal:
    return llvm::CallingConv::X86_Pascal;
  // TODO: Add support for __vectorcall to LLVM.
  case CC_X86VectorCall:
    return llvm::CallingConv::X86_VectorCall;
  case CC_AArch64VectorCall:
    return llvm::CallingConv::AArch64_VectorCall;
  case CC_AArch64SVEPCS:
    return llvm::CallingConv::AArch64_SVE_VectorCall;
  case CC_SpirFunction:
    return llvm::CallingConv::SPIR_FUNC;
  case CC_DeviceKernel:
    return CGM.getTargetCodeGenInfo().getDeviceKernelCallingConv();
  case CC_PreserveMost:
    return llvm::CallingConv::PreserveMost;
  case CC_PreserveAll:
    return llvm::CallingConv::PreserveAll;
  case CC_Swift:
    return llvm::CallingConv::Swift;
  case CC_SwiftAsync:
    return llvm::CallingConv::SwiftTail;
  case CC_M68kRTD:
    return llvm::CallingConv::M68k_RTD;
  case CC_PreserveNone:
    return llvm::CallingConv::PreserveNone;
    // clang-format off
  case CC_RISCVVectorCall: return llvm::CallingConv::RISCV_VectorCall;
    // clang-format on
#define CC_VLS_CASE(ABI_VLEN)                                                  \
  case CC_RISCVVLSCall_##ABI_VLEN:                                             \
    return llvm::CallingConv::RISCV_VLSCall_##ABI_VLEN;
    CC_VLS_CASE(32)
    CC_VLS_CASE(64)
    CC_VLS_CASE(128)
    CC_VLS_CASE(256)
    CC_VLS_CASE(512)
    CC_VLS_CASE(1024)
    CC_VLS_CASE(2048)
    CC_VLS_CASE(4096)
    CC_VLS_CASE(8192)
    CC_VLS_CASE(16384)
    CC_VLS_CASE(32768)
    CC_VLS_CASE(65536)
#undef CC_VLS_CASE
  }
}

/// Derives the 'this' type for codegen purposes, i.e. ignoring method CVR
/// qualification. Either or both of RD and MD may be null. A null RD indicates
/// that there is no meaningful 'this' type, and a null MD can occur when
/// calling a method pointer.
CanQualType CodeGenTypes::DeriveThisType(const CXXRecordDecl *RD,
                                         const CXXMethodDecl *MD) {
  CanQualType RecTy;
  if (RD)
    RecTy = Context.getCanonicalTagType(RD);
  else
    RecTy = Context.VoidTy;

  if (MD)
    RecTy = CanQualType::CreateUnsafe(Context.getAddrSpaceQualType(
        RecTy, MD->getMethodQualifiers().getAddressSpace()));
  return Context.getPointerType(RecTy);
}

/// Returns the canonical formal type of the given C++ method.
static CanQual<FunctionProtoType> GetFormalType(const CXXMethodDecl *MD) {
  return MD->getType()
      ->getCanonicalTypeUnqualified()
      .getAs<FunctionProtoType>();
}

/// Returns the "extra-canonicalized" return type, which discards
/// qualifiers on the return type.  Codegen doesn't care about them,
/// and it makes ABI code a little easier to be able to assume that
/// all parameter and return types are top-level unqualified.
static CanQualType GetReturnType(QualType RetTy) {
  return RetTy->getCanonicalTypeUnqualified();
}

/// Arrange the argument and result information for a value of the given
/// unprototyped freestanding function type.
const CGFunctionInfo &
CodeGenTypes::arrangeFreeFunctionType(CanQual<FunctionNoProtoType> FTNP) {
  // When translating an unprototyped function type, always use a
  // variadic type.
  return arrangeLLVMFunctionInfo(FTNP->getReturnType().getUnqualifiedType(),
                                 FnInfoOpts::None, {}, FTNP->getExtInfo(), {},
                                 RequiredArgs(0), /*ABIInfoFD=*/nullptr);
}

static void addExtParameterInfosForCall(
    llvm::SmallVectorImpl<FunctionProtoType::ExtParameterInfo> &paramInfos,
    const FunctionProtoType *proto, unsigned prefixArgs, unsigned totalArgs) {
  assert(proto->hasExtParameterInfos());
  assert(paramInfos.size() <= prefixArgs);
  assert(proto->getNumParams() + prefixArgs <= totalArgs);

  paramInfos.reserve(totalArgs);

  // Add default infos for any prefix args that don't already have infos.
  paramInfos.resize(prefixArgs);

  // Add infos for the prototype.
  for (const auto &ParamInfo : proto->getExtParameterInfos()) {
    paramInfos.push_back(ParamInfo);
    // pass_object_size params have no parameter info.
    if (ParamInfo.hasPassObjectSize())
      paramInfos.emplace_back();
  }

  assert(paramInfos.size() <= totalArgs &&
         "Did we forget to insert pass_object_size args?");
  // Add default infos for the variadic and/or suffix arguments.
  paramInfos.resize(totalArgs);
}

/// Adds the formal parameters in FPT to the given prefix. If any parameter in
/// FPT has pass_object_size attrs, then we'll add parameters for those, too.
static void appendParameterTypes(
    const CodeGenTypes &CGT, SmallVectorImpl<CanQualType> &prefix,
    SmallVectorImpl<FunctionProtoType::ExtParameterInfo> &paramInfos,
    CanQual<FunctionProtoType> FPT) {
  // Fast path: don't touch param info if we don't need to.
  if (!FPT->hasExtParameterInfos()) {
    assert(paramInfos.empty() &&
           "We have paramInfos, but the prototype doesn't?");
    prefix.append(FPT->param_type_begin(), FPT->param_type_end());
    return;
  }

  unsigned PrefixSize = prefix.size();
  // In the vast majority of cases, we'll have precisely FPT->getNumParams()
  // parameters; the only thing that can change this is the presence of
  // pass_object_size. So, we preallocate for the common case.
  prefix.reserve(prefix.size() + FPT->getNumParams());

  auto ExtInfos = FPT->getExtParameterInfos();
  assert(ExtInfos.size() == FPT->getNumParams());
  for (unsigned I = 0, E = FPT->getNumParams(); I != E; ++I) {
    prefix.push_back(FPT->getParamType(I));
    if (ExtInfos[I].hasPassObjectSize())
      prefix.push_back(CGT.getContext().getCanonicalSizeType());
  }

  addExtParameterInfosForCall(paramInfos, FPT.getTypePtr(), PrefixSize,
                              prefix.size());
}

using ExtParameterInfoList =
    SmallVector<FunctionProtoType::ExtParameterInfo, 16>;

/// Arrange the LLVM function layout for a value of the given function
/// type, on top of any implicit parameters already stored.
static const CGFunctionInfo &
arrangeLLVMFunctionInfo(CodeGenTypes &CGT, bool instanceMethod,
                        SmallVectorImpl<CanQualType> &prefix,
                        CanQual<FunctionProtoType> FTP) {
  ExtParameterInfoList paramInfos;
  RequiredArgs Required = RequiredArgs::forPrototypePlus(FTP, prefix.size());
  appendParameterTypes(CGT, prefix, paramInfos, FTP);
  CanQualType resultType = FTP->getReturnType().getUnqualifiedType();

  FnInfoOpts opts =
      instanceMethod ? FnInfoOpts::IsInstanceMethod : FnInfoOpts::None;
  return CGT.arrangeLLVMFunctionInfo(resultType, opts, prefix,
                                     FTP->getExtInfo(), paramInfos, Required,
                                     /*ABIInfoFD=*/nullptr);
}

using CanQualTypeList = SmallVector<CanQualType, 16>;

/// Arrange the argument and result information for a value of the given
/// freestanding function type.
const CGFunctionInfo &
CodeGenTypes::arrangeFreeFunctionType(CanQual<FunctionProtoType> FTP) {
  CanQualTypeList argTypes;
  return ::arrangeLLVMFunctionInfo(*this, /*instanceMethod=*/false, argTypes,
                                   FTP);
}

static CallingConv getCallingConventionForDecl(const ObjCMethodDecl *D,
                                               bool IsTargetDefaultMSABI) {
  // Set the appropriate calling convention for the Function.
  if (D->hasAttr<StdCallAttr>())
    return CC_X86StdCall;

  if (D->hasAttr<FastCallAttr>())
    return CC_X86FastCall;

  if (D->hasAttr<RegCallAttr>())
    return CC_X86RegCall;

  if (D->hasAttr<ThisCallAttr>())
    return CC_X86ThisCall;

  if (D->hasAttr<VectorCallAttr>())
    return CC_X86VectorCall;

  if (D->hasAttr<PascalAttr>())
    return CC_X86Pascal;

  if (PcsAttr *PCS = D->getAttr<PcsAttr>())
    return (PCS->getPCS() == PcsAttr::AAPCS ? CC_AAPCS : CC_AAPCS_VFP);

  if (D->hasAttr<AArch64VectorPcsAttr>())
    return CC_AArch64VectorCall;

  if (D->hasAttr<AArch64SVEPcsAttr>())
    return CC_AArch64SVEPCS;

  if (D->hasAttr<DeviceKernelAttr>())
    return CC_DeviceKernel;

  if (D->hasAttr<IntelOclBiccAttr>())
    return CC_IntelOclBicc;

  if (D->hasAttr<MSABIAttr>())
    return IsTargetDefaultMSABI ? CC_C : CC_Win64;

  if (D->hasAttr<SysVABIAttr>())
    return IsTargetDefaultMSABI ? CC_X86_64SysV : CC_C;

  if (D->hasAttr<PreserveMostAttr>())
    return CC_PreserveMost;

  if (D->hasAttr<PreserveAllAttr>())
    return CC_PreserveAll;

  if (D->hasAttr<M68kRTDAttr>())
    return CC_M68kRTD;

  if (D->hasAttr<PreserveNoneAttr>())
    return CC_PreserveNone;

  if (D->hasAttr<RISCVVectorCCAttr>())
    return CC_RISCVVectorCall;

  if (RISCVVLSCCAttr *PCS = D->getAttr<RISCVVLSCCAttr>()) {
    switch (PCS->getVectorWidth()) {
    default:
      llvm_unreachable("Invalid RISC-V VLS ABI VLEN");
#define CC_VLS_CASE(ABI_VLEN)                                                  \
  case ABI_VLEN:                                                               \
    return CC_RISCVVLSCall_##ABI_VLEN;
      CC_VLS_CASE(32)
      CC_VLS_CASE(64)
      CC_VLS_CASE(128)
      CC_VLS_CASE(256)
      CC_VLS_CASE(512)
      CC_VLS_CASE(1024)
      CC_VLS_CASE(2048)
      CC_VLS_CASE(4096)
      CC_VLS_CASE(8192)
      CC_VLS_CASE(16384)
      CC_VLS_CASE(32768)
      CC_VLS_CASE(65536)
#undef CC_VLS_CASE
    }
  }

  return CC_C;
}

/// Arrange a call to a C++ method, passing the given arguments.
///
/// ExtraPrefixArgs is the number of ABI-specific args passed after the `this`
/// parameter.
/// ExtraSuffixArgs is the number of ABI-specific args passed at the end of
/// args.
/// PassProtoArgs indicates whether `args` has args for the parameters in the
/// given CXXConstructorDecl.
const CGFunctionInfo &CodeGenTypes::arrangeCXXConstructorCall(
    const CallArgList &args, const CXXConstructorDecl *D, CXXCtorType CtorKind,
    unsigned ExtraPrefixArgs, unsigned ExtraSuffixArgs,
    const FunctionDecl *ABIInfoFD, bool PassProtoArgs) {
  CanQualTypeList ArgTypes;
  for (const auto &Arg : args)
    ArgTypes.push_back(Context.getCanonicalParamType(Arg.Ty));

  // +1 for implicit this, which should always be args[0].
  unsigned TotalPrefixArgs = 1 + ExtraPrefixArgs;

  CanQual<FunctionProtoType> FPT = GetFormalType(D);
  RequiredArgs Required = PassProtoArgs
                              ? RequiredArgs::forPrototypePlus(
                                    FPT, TotalPrefixArgs + ExtraSuffixArgs)
                              : RequiredArgs::All;

  GlobalDecl GD(D, CtorKind);
  CanQualType ResultType = getCXXABI().HasThisReturn(GD) ? ArgTypes.front()
                           : getCXXABI().hasMostDerivedReturn(GD)
                               ? CGM.getContext().VoidPtrTy
                               : Context.VoidTy;

  FunctionType::ExtInfo Info = FPT->getExtInfo();
  ExtParameterInfoList ParamInfos;
  // If the prototype args are elided, we should only have ABI-specific args,
  // which never have param info.
  if (PassProtoArgs && FPT->hasExtParameterInfos()) {
    // ABI-specific suffix arguments are treated the same as variadic arguments.
    addExtParameterInfosForCall(ParamInfos, FPT.getTypePtr(), TotalPrefixArgs,
                                ArgTypes.size());
  }

  return arrangeLLVMFunctionInfo(ResultType, FnInfoOpts::IsInstanceMethod,
                                 ArgTypes, Info, ParamInfos, Required,
                                 ABIInfoFD);
}

// The remainder of this file is unchanged from the current repository version.
