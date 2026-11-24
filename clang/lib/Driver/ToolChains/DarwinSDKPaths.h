//===--- DarwinSDKPaths.h - Darwin SDK path handling ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_DARWINSDKPATHS_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_DARWINSDKPATHS_H

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Path.h"

namespace clang::driver::tools::darwin {

// Configured system include paths are interpreted relative to the selected
// Darwin SDK, whether written with a leading slash or without one. With no
// SDK, the effective sysroot is "/", retaining native system include paths.
inline llvm::SmallString<128> getSDKIncludePath(llvm::StringRef Sysroot,
                                               llvm::StringRef Dir) {
  llvm::SmallString<128> Path(Sysroot);
  llvm::sys::path::append(Path, Dir.ltrim('/'));
  return Path;
}

} // namespace clang::driver::tools::darwin

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_DARWINSDKPATHS_H
