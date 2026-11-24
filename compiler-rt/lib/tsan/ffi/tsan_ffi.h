//===-- tsan_ffi.h ----------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_FFI_H
#define TSAN_FFI_H

#include "sanitizer_common/sanitizer_internal_defs.h"
#if __has_include(<ffi.h>)
#  include <ffi.h>
#else
#  include <ffi/ffi.h>
#endif

namespace __tsan {

using __sanitizer::uptr;

// Logical scalar payloads, not register slots or pointed-to objects.
inline uptr FfiScalarSize(const ffi_type *type) {
  if (!type)
    return 0;
  uptr expected;
  switch (type->type) {
    case FFI_TYPE_UINT8:
    case FFI_TYPE_SINT8:
      expected = 1;
      break;
    case FFI_TYPE_UINT16:
    case FFI_TYPE_SINT16:
      expected = 2;
      break;
    case FFI_TYPE_UINT32:
    case FFI_TYPE_SINT32:
    case FFI_TYPE_FLOAT:
      expected = 4;
      break;
    case FFI_TYPE_UINT64:
    case FFI_TYPE_SINT64:
    case FFI_TYPE_DOUBLE:
      expected = 8;
      break;
    case FFI_TYPE_POINTER:
      expected = sizeof(void *);
      break;
    default:
      return 0;
  }
  return type->size == expected ? expected : 0;
}

// libffi requires word-sized storage for integral returns narrower than
// ffi_arg. Float returns retain their own size. Only use for the native
// default ABI, whose return stores have been reviewed.
inline uptr FfiScalarResultSize(const ffi_type *type) {
  const uptr size = FfiScalarSize(type);
  if (!size)
    return 0;
  switch (type->type) {
    case FFI_TYPE_UINT8:
    case FFI_TYPE_SINT8:
    case FFI_TYPE_UINT16:
    case FFI_TYPE_SINT16:
    case FFI_TYPE_UINT32:
    case FFI_TYPE_SINT32:
      return size < sizeof(ffi_arg) ? sizeof(ffi_arg) : size;
    default:
      return size;
  }
}

#if SANITIZER_APPLE && defined(__aarch64__) && !SANITIZER_GO
// Native Darwin AArch64, flat homogeneous float/double aggregates only.
uptr FfiAarch64HfaSize(const ffi_type *type);
#endif

}  // namespace __tsan

#endif  // TSAN_FFI_H
