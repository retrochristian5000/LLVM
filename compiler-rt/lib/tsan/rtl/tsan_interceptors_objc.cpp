//===-- tsan_interceptors_objc.cpp ----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Objective-C runtime synchronization interceptors shared by Darwin targets.

#include "sanitizer_common/sanitizer_platform.h"
#if SANITIZER_APPLE && !SANITIZER_GO

#include <objc/objc-sync.h>
#include <objc/objc.h>

#include "interception/interception.h"
#include "tsan_interceptors.h"
#include "tsan_sync.h"

namespace __tsan {

TSAN_INTERCEPTOR(int, objc_sync_enter, id obj) {
  SCOPED_TSAN_INTERCEPTOR(objc_sync_enter, obj);
  if (!obj)
    return REAL(objc_sync_enter)(obj);
  uptr addr = SyncAddressForObjCObject((uptr)obj, thr, pc);
  MutexPreLock(thr, pc, addr, MutexFlagWriteReentrant);
  int result = REAL(objc_sync_enter)(obj);
  if (result == OBJC_SYNC_SUCCESS)
    MutexPostLock(thr, pc, addr, MutexFlagWriteReentrant);
  return result;
}

TSAN_INTERCEPTOR(int, objc_sync_exit, id obj) {
  SCOPED_TSAN_INTERCEPTOR(objc_sync_exit, obj);
  if (!obj)
    return REAL(objc_sync_exit)(obj);
  uptr addr = SyncAddressForObjCObject((uptr)obj, thr, pc);
  MutexUnlock(thr, pc, addr);
  int result = REAL(objc_sync_exit)(obj);
  if (result != OBJC_SYNC_SUCCESS)
    MutexInvalidAccess(thr, pc, addr);
  return result;
}

// objc_sync_try_enter is exported by libobjc starting with macOS 10.15 and
// iOS 13. Use the Objective-C BOOL ABI (signed char on x86_64 macOS,
// bool on arm64), and weak-link the optional entry point for older systems.
#if defined(__MAC_10_15) || defined(__IPHONE_13_0) || defined(__TVOS_13_0) || \
    defined(__WATCHOS_6_0) || defined(__VISIONOS_1_0)
extern "C" BOOL objc_sync_try_enter(id obj) __attribute__((weak_import));

TSAN_INTERCEPTOR(BOOL, objc_sync_try_enter, id obj) {
  SCOPED_TSAN_INTERCEPTOR(objc_sync_try_enter, obj);
  const BOOL result = REAL(objc_sync_try_enter)(obj);
  if (result && obj) {
    const uptr addr = SyncAddressForObjCObject((uptr)obj, thr, pc);
    MutexPostLock(thr, pc, addr,
                  MutexFlagWriteReentrant | MutexFlagTryLock);
  }
  return result;
}
#endif

}  // namespace __tsan

#endif  // SANITIZER_APPLE && !SANITIZER_GO
