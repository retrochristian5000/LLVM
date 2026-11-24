//===-- tsan_sync_darwin.cpp ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Darwin-specific synchronization address handling for Objective-C objects.
// The objc_sync interceptors live in tsan_interceptors_objc.cpp;
// shared SyncVar and MetaMap logic stays in tsan_sync.cpp.

#include "sanitizer_common/sanitizer_platform.h"
#if SANITIZER_APPLE

#include "sanitizer_common/sanitizer_addrhashmap.h"
#include "../mman/tsan_mman.h"
#include "tsan_rtl.h"
#include "tsan_sync.h"

namespace __tsan {

// Determines whether the Obj-C object pointer is a tagged pointer. Tagged
// pointers encode the object data directly in their pointer bits and do not
// have an associated memory allocation. The Obj-C runtime uses tagged pointers
// to transparently optimize small objects.
bool IsTaggedObjCPointer(uptr obj) {
  // Match objc4's tag marker, not the tag payload. ARM64/ARM64e uses the
  // most significant bit even in the split-tag representation. Desktop
  // x86_64 (including Catalyst) uses the least significant bit.
#if defined(__x86_64__) && (SANITIZER_OSX || TARGET_OS_MACCATALYST)
  constexpr uptr kTagMask = 1;
#else
  constexpr uptr kTagMask = 1ull << 63;
#endif
  return (obj & kTagMask) != 0;
}

// Returns an address which can be used to inform TSan about synchronization
// points (MutexLock/Unlock). The TSan infrastructure expects this to be a valid
// address in the process space. We do a small allocation here to obtain a
// stable address (the array backing the hash map can change). The memory is
// never free'd (leaked) and allocation and locking are slow, but this code only
// runs for @synchronized with tagged pointers, which is very rare.
static uptr GetOrCreateSyncAddress(uptr addr, ThreadState *thr, uptr pc) {
  typedef AddrHashMap<uptr, 5> Map;
  static Map Addresses;
  Map::Handle h(&Addresses, addr);
  if (h.created()) {
    ThreadIgnoreBegin(thr, pc);
    *h = (uptr)user_alloc(thr, pc, /*size=*/1);
    ThreadIgnoreEnd(thr);
  }
  return *h;
}

// Returns an address on which we can synchronize given an Obj-C object pointer.
// For normal object pointers, this is just the address of the object in memory.
// Tagged pointers are not backed by an actual memory allocation, so we need to
// synthesize a valid address.
uptr SyncAddressForObjCObject(uptr obj, ThreadState *thr, uptr pc) {
  if (IsTaggedObjCPointer(obj))
    return GetOrCreateSyncAddress(obj, thr, pc);
  return obj;
}

}  // namespace __tsan

#endif  // SANITIZER_APPLE
