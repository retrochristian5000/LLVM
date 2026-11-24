//===-- tsan_objc_runtime.m -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tsan_objc_runtime.h"

#include <dlfcn.h>
#include <objc/runtime.h>
#if __has_feature(ptrauth_intrinsics)
#  include <ptrauth.h>
#endif

int __tsan_objc_query_image(TsanObjCImagePlacement *placement) {
  if (!placement)
    return 0;

  // Do not invoke the runtime function, initialize classes or send messages.
  // Strip only a diagnostic copy of a code address, never a callable IMP.
#if __has_feature(ptrauth_intrinsics)
  const uintptr_t pc =
      (uintptr_t)ptrauth_strip(&class_getImageName, ptrauth_key_function_pointer);
#else
  const uintptr_t pc = (uintptr_t)&class_getImageName;
#endif
  Dl_info info = {0};
  if (!pc || !dladdr((const void *)pc, &info) ||
      !info.dli_fbase || !info.dli_fname)
    return 0;

  *placement = (TsanObjCImagePlacement){
      .base = (uintptr_t)info.dli_fbase,
      .pc = pc,
      .image_name = info.dli_fname,
      .bool_encoding = @encode(BOOL),
      .bool_size = sizeof(BOOL),
  };
  return 1;
}
