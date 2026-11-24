//===-- tsan_objc_runtime.h ------------------------------------------*- C -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_OBJC_RUNTIME_H
#define TSAN_OBJC_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Private C ABI. Addresses are diagnostic values, never callable pointers.
// The image name and type encoding are borrowed from process-lifetime data.
typedef struct {
  uintptr_t base;
  uintptr_t pc;
  const char *image_name;
  const char *bool_encoding;
  size_t bool_size;
} TsanObjCImagePlacement;

__attribute__((visibility("hidden")))
int __tsan_objc_query_image(TsanObjCImagePlacement *placement);

#ifdef __cplusplus
}
#endif

#endif  // TSAN_OBJC_RUNTIME_H
