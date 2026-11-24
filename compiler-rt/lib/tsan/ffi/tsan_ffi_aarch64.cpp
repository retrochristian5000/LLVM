//===-- tsan_ffi_aarch64.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE && defined(__aarch64__) && !SANITIZER_GO
#  include "tsan_ffi.h"
#  include "../rtl/tsan_platform.h"

namespace __tsan {

// The descriptor must already have been prepared by libffi. This recognizes
// only a padding-free, flat HFA of one to four float or double members.
// The provider decides register versus stack placement and owns all copies.
// Do not decode private cif flags, inspect register slots, or strip PAC.
uptr FfiAarch64HfaSize(const ffi_type *type) {
  if (!type ||
      !IsValidAppMemoryRange(reinterpret_cast<uptr>(type), sizeof(*type)) ||
      type->type != FFI_TYPE_STRUCT || !type->elements)
    return 0;

  const uptr size = type->size;
  if (size < sizeof(float) || size > 4 * sizeof(double))
    return 0;

  ffi_type *const *elements = type->elements;
  const uptr base = reinterpret_cast<uptr>(elements);
  uptr member_size = 0;
  unsigned member_kind = 0;
  uptr member_alignment = 0;

  // Check each slot before reading it. The fifth slot may only terminate
  // the array; never recurse into a nested or unknown aggregate.
  for (uptr i = 0; i <= 4; ++i) {
    const uptr offset = i * sizeof(*elements);
    if (base > ~uptr(0) - offset ||
        !IsValidAppMemoryRange(base + offset, sizeof(*elements)))
      return 0;
    const ffi_type *member = elements[i];
    if (!member)
      return i && size == i * member_size &&
                     type->alignment == member_alignment
                 ? size
                 : 0;
    if (i == 4 ||
        !IsValidAppMemoryRange(reinterpret_cast<uptr>(member), sizeof(*member)))
      return 0;

    const unsigned kind = member->type;
    uptr expected_size, expected_alignment;
    if (kind == FFI_TYPE_FLOAT) {
      expected_size = sizeof(float);
      expected_alignment = alignof(float);
    } else if (kind == FFI_TYPE_DOUBLE) {
      expected_size = sizeof(double);
      expected_alignment = alignof(double);
    } else {
      return 0;
    }
    if (member->size != expected_size ||
        member->alignment != expected_alignment)
      return 0;
    if (i && kind != member_kind)
      return 0;
    member_kind = kind;
    member_size = expected_size;
    member_alignment = expected_alignment;
  }
  return 0;
}

}  // namespace __tsan

#endif  // SANITIZER_APPLE && defined(__aarch64__) && !SANITIZER_GO
