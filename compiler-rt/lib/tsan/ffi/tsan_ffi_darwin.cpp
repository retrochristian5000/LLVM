//===-- tsan_ffi_darwin.cpp ------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Track scalar argument reads and result writes by uninstrumented libffi.
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

// Only model application ranges; never dereference a payload or strip PAC.
void TrackRange(ThreadState *thr, uptr pc, const void *ptr, uptr size,
                bool write) {
  const uptr addr = reinterpret_cast<uptr>(ptr);
  if (!size || !addr || addr > ~uptr(0) - size)
    return;
  if (!IsAppMem(addr) || !IsAppMem(addr + size - 1))
    return;
  MemoryAccessRange(thr, pc, addr, size, write);
}

void ReadArguments(ThreadState *thr, uptr pc, const ffi_cif *cif,
                   void **values) {
  if (!cif || !values || !cif->arg_types)
    return;
  for (unsigned i = 0; i < cif->nargs; ++i) {
    // The pointer slot is read even when its payload type is unsupported.
    TrackRange(thr, pc, &values[i], sizeof(values[i]), false);
    const uptr size = FfiScalarSize(cif->arg_types[i]);
    if (size)
      TrackRange(thr, pc, values[i], size, false);
  }
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
      if (cif && result && cif->abi == FFI_DEFAULT_ABI)
        result_size = FfiScalarResultSize(cif->rtype);
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
