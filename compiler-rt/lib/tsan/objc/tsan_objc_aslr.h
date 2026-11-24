//===-- tsan_objc_aslr.h -----------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_OBJC_ASLR_H
#define TSAN_OBJC_ASLR_H

namespace __tsan {

// Startup diagnostics only; dyld owns relocation and the VM owns ASLR.
void ReportObjCImagePlacement();

}  // namespace __tsan

#endif  // TSAN_OBJC_ASLR_H
