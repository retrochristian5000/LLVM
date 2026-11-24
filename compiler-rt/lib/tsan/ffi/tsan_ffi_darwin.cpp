//===-- tsan_ffi_darwin.cpp ------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Track supported argument reads and result writes by uninstrumented libffi.
// The provider still owns the calling convention and executable trampolines.
//
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE && !SANITIZER_GO
#  include "tsan_ffi.h"

#  include "../rtl/tsan_interceptors.h"
#  include "../rtl/tsan_platform.h"
#  include "interception/interception.h"

using namespace __tsan;

namespace {

// A valid TSan range is not proof that an address is mapped or readable.
// libffi still requires the caller to supply live, prepared metadata. If a
// range cannot be represented in TSan, leave the actual call to the provider.
bool CanInspect(const void *ptr, uptr size) {
  return ptr && IsValidAppMemoryRange(reinterpret_cast<uptr>(ptr), size);
}

bool CanTrack(uptr addr, uptr size) {
  uptr begin, end;
  // Check raw addresses first. Do not normalize unsupported Darwin pointers.
  return IsValidAppMemoryRange(addr, size) &&
         GetShadowCellRange(addr, size, &begin, &end);
}

void TrackRange(ThreadState *thr, uptr pc, const void *ptr, uptr size,
                bool write) {
  uptr addr = reinterpret_cast<uptr>(ptr);
  if (!ptr || !CanTrack(addr, size))
    return;

  // Track exact bytes, not rounded shadow cells. Process each cell separately:
  // a read-only first cell must not suppress a later writable cell, and the
  // next application's shadow address need not be adjacent at a mapping edge.
  while (size) {
    const uptr chunk = Min(size, kShadowCell - (addr % kShadowCell));
    MemoryAccessRange(thr, pc, addr, chunk, write);
    addr += chunk;
    size -= chunk;
  }
}

uptr ArgumentSize(const ffi_type *type, ffi_abi abi) {
  const uptr scalar = FfiScalarSize(type);
  if (scalar)
    return scalar;
#if defined(__aarch64__)
  if (abi == FFI_DEFAULT_ABI)
    return FfiAarch64HfaSize(type);
#else
  (void)abi;
#endif
  return 0;
}

void ReadArguments(ThreadState *thr, uptr pc, const ffi_cif *cif,
                   void **values) {
  if (!CanInspect(cif, sizeof(*cif)))
    return;

  const uptr count = cif->nargs;
  if (!count)
    return;
  ffi_type *const *types = cif->arg_types;
  if (count > ~uptr(0) / sizeof(*values) ||
      count > ~uptr(0) / sizeof(*types))
    return;
  if (!CanInspect(values, count * sizeof(*values)) ||
      !CanInspect(types, count * sizeof(*types)))
    return;

  for (uptr i = 0; i < count; ++i) {
    // The pointer slot is read even when its payload type is unsupported.
    TrackRange(thr, pc, &values[i], sizeof(values[i]), false);
    const ffi_type *type = types[i];
    if (!CanInspect(type, sizeof(*type)))
      continue;
    const uptr size = ArgumentSize(type, cif->abi);
    if (size)
      TrackRange(thr, pc, values[i], size, false);
  }
}

uptr ResultSize(const ffi_cif *cif, const void *result) {
  if (!result || !CanInspect(cif, sizeof(*cif)) ||
      cif->abi != FFI_DEFAULT_ABI)
    return 0;
  const ffi_type *type = cif->rtype;
  if (!CanInspect(type, sizeof(*type)))
    return 0;
  const uptr scalar = FfiScalarResultSize(type);
  if (scalar)
    return scalar;
#if defined(__aarch64__)
  return FfiAarch64HfaSize(type);
#else
  return 0;
#endif
}

}  // namespace

TSAN_INTERCEPTOR(void, ffi_call, ffi_cif *cif, void (*fn)(void), void *result,
                 void **values) {
  uptr result_size = 0;
  const uptr caller_pc = GET_CALLER_PC();
  {
    SCOPED_INTERCEPTOR_RAW(ffi_call);
    if (!MustIgnoreInterceptor(thr)) {
      ReadArguments(thr, pc, cif, values);
      result_size = ResultSize(cif, result);
    }
  }
  // Do not keep interceptor state across arbitrary foreign code. It may
  // call instrumented code, throw an exception, or perform a nonlocal jump.
  // The call does not establish a happens-before edge.
  REAL(ffi_call)(cif, fn, result, values);
  // Only a normally returned call has produced a result. Re-enter TSan
  // without retaining a scope across the foreign function or rereading cif.
  if (result_size) {
    ThreadState *thr = cur_thread_init();
    ScopedInterceptor si(thr, "ffi_call", caller_pc);
    if (!MustIgnoreInterceptor(thr))
      TrackRange(thr, GET_CURRENT_PC(), result, result_size, true);
  }
}

#endif  // SANITIZER_APPLE && !SANITIZER_GO
