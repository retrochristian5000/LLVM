//===-- tsan_interface.cpp ------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
//===----------------------------------------------------------------------===//

#include "tsan_interface.h"
#include "tsan_interface_ann.h"
#include "tsan_interface_aarch64.h"
#include "../rtl/tsan_rtl.h"
#include "sanitizer_common/sanitizer_internal_defs.h"
#include "sanitizer_common/sanitizer_ptrauth.h"

#define CALLERPC ((uptr)__builtin_return_address(0))

#if defined(__aarch64__) && !SANITIZER_GO
// Fiber calls are relatively infrequent. Normalize their caller PCs at the
// AArch64 ABI boundary without slowing down memory-access instrumentation.
#  define FIBER_CALLERPC \
    ((uptr)__tsan_interface_aarch64_strip_pc(__builtin_return_address(0)))
#else
#  define FIBER_CALLERPC CALLERPC
#endif

using namespace __tsan;

void __tsan_init() { Initialize(cur_thread_init()); }

void __tsan_flush_memory() {
  FlushShadowMemory();
}

namespace {

// An explicit-PC caller may describe a byte range rather than a naturally
// aligned C++ object. Each eight-byte access must stay within its shadow
// cell, or the shadow mask silently drops bytes at the end of that cell.
void Access16WithPC(void *address, void *caller_pc, AccessType type) {
  const uptr addr = reinterpret_cast<uptr>(address);
  const uptr pc = STRIP_PAC_PC(caller_pc);
  ThreadState *thr = cur_thread();
  if (addr % kShadowCell) {
    UnalignedMemoryAccess(thr, pc, addr, 8, type);
    UnalignedMemoryAccess(thr, pc, addr + 8, 8, type);
  } else {
    MemoryAccess(thr, pc, addr, 8, type);
    MemoryAccess(thr, pc, addr + 8, 8, type);
  }
}

}  // namespace

void __tsan_read16_pc(void *addr, void *pc) {
  Access16WithPC(addr, pc, kAccessRead);
}

void __tsan_write16_pc(void *addr, void *pc) {
  Access16WithPC(addr, pc, kAccessWrite);
}

// __tsan_unaligned_read/write calls are emitted by compiler.

void __tsan_unaligned_read16(const void *addr) {
  uptr pc = CALLERPC;
  ThreadState *thr = cur_thread();
  UnalignedMemoryAccess(thr, pc, (uptr)addr, 8, kAccessRead);
  UnalignedMemoryAccess(thr, pc, (uptr)addr + 8, 8, kAccessRead);
}

void __tsan_unaligned_write16(void *addr) {
  uptr pc = CALLERPC;
  ThreadState *thr = cur_thread();
  UnalignedMemoryAccess(thr, pc, (uptr)addr, 8, kAccessWrite);
  UnalignedMemoryAccess(thr, pc, (uptr)addr + 8, 8, kAccessWrite);
}

extern "C" {
SANITIZER_INTERFACE_ATTRIBUTE
void *__tsan_get_current_fiber() {
  return cur_thread();
}

SANITIZER_INTERFACE_ATTRIBUTE
void *__tsan_create_fiber(unsigned flags) {
  return FiberCreate(cur_thread(), FIBER_CALLERPC, flags);
}

SANITIZER_INTERFACE_ATTRIBUTE
void __tsan_destroy_fiber(void *fiber) {
  FiberDestroy(cur_thread(), FIBER_CALLERPC, static_cast<ThreadState *>(fiber));
}

SANITIZER_INTERFACE_ATTRIBUTE
void __tsan_switch_to_fiber(void *fiber, unsigned flags) {
  FiberSwitch(cur_thread(), FIBER_CALLERPC, static_cast<ThreadState *>(fiber), flags);
}

SANITIZER_INTERFACE_ATTRIBUTE
void __tsan_set_fiber_name(void *fiber, const char *name) {
  ThreadSetName(static_cast<ThreadState *>(fiber), name);
}
}  // extern "C"

#undef FIBER_CALLERPC

void __tsan_acquire(void *addr) {
  Acquire(cur_thread(), CALLERPC, (uptr)addr);
}

void __tsan_release(void *addr) {
  Release(cur_thread(), CALLERPC, (uptr)addr);
}
