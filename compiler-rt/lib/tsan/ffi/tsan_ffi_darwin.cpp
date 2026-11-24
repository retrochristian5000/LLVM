//===-- tsan_ffi_darwin.cpp ------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Track logical scalar argument reads performed by an uninstrumented libffi.
// The provider still owns the calling convention and executable trampolines.
//
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE && !SANITIZER_GO
#  if __has_include(<ffi.h>)
#    include <ffi.h>
#  else
#    include <ffi/ffi.h>
#  endif

#  include "../rtl/tsan_interceptors.h"
#  include "../rtl/tsan_platform.h"
#  include "interception/interception.h"

using namespace __tsan;

namespace {

// Only account for fixed-width scalar payloads whose logical read size is
// unambiguous. Aggregate, vector, narrow and return storage need ABI-specific
// review; never infer the size of a pointee from a pointer argument.
uptr ScalarReadSize(const ffi_type *type) {
  if (!type)
    return 0;
  switch (type->type) {
    case FFI_TYPE_UINT64:
    case FFI_TYPE_SINT64:
    case FFI_TYPE_DOUBLE:
      return type->size == 8 ? 8 : 0;
    case FFI_TYPE_POINTER:
      return type->size == sizeof(void *) ? sizeof(void *) : 0;
    default:
      return 0;
  }
}

void ReadArguments(ThreadState *thr, uptr pc, const ffi_cif *cif,
                   void **values) {
  if (!cif || !values || !cif->arg_types)
    return;
  for (unsigned i = 0; i < cif->nargs; ++i) {
    const uptr size = ScalarReadSize(cif->arg_types[i]);
    const uptr addr = reinterpret_cast<uptr>(values[i]);
    if (!size || !addr || addr > ~uptr(0) - (size - 1))
      continue;
    if (!IsAppMem(addr) || !IsAppMem(addr + size - 1))
      continue;
    MemoryAccessRange(thr, pc, addr, size, false);
  }
}

}  // namespace

TSAN_INTERCEPTOR(void, ffi_call, ffi_cif *cif, void (*fn)(void), void *result,
                 void **values) {
  {
    SCOPED_INTERCEPTOR_RAW(ffi_call);
    if (!MustIgnoreInterceptor(thr))
      ReadArguments(thr, pc, cif, values);
  }
  // Do not keep interceptor state across arbitrary foreign code. It may
  // call instrumented code, throw an exception, or perform a nonlocal jump.
  // The call does not establish a happens-before edge.
  REAL(ffi_call)(cif, fn, result, values);
}

#endif  // SANITIZER_APPLE && !SANITIZER_GO
