//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_TARGET_TARGETAPILOCK_H
#define LLDB_TARGET_TARGETAPILOCK_H

#include <mutex>

namespace lldb_private {

/// A Lockable handle returned by Target::GetAPIMutex(). Wraps a pointer to
/// a real, persistent std::recursive_mutex (Target::m_mutex or
/// Target::m_private_mutex), or is a genuine no-op -- no synchronization
/// primitive touched at all -- when default-constructed. Satisfies
/// BasicLockable/Lockable, so it's used exactly like std::recursive_mutex
/// as the template argument to std::lock_guard<T>/std::unique_lock<T>.
class TargetAPILock {
public:
  TargetAPILock() = default;
  explicit TargetAPILock(std::recursive_mutex &mutex) : m_mutex(&mutex) {}

  void lock() {
    if (m_mutex)
      m_mutex->lock();
  }
  void unlock() {
    if (m_mutex)
      m_mutex->unlock();
  }
  bool try_lock() { return m_mutex ? m_mutex->try_lock() : true; }

private:
  std::recursive_mutex *m_mutex = nullptr;
};

} // namespace lldb_private

#endif // LLDB_TARGET_TARGETAPILOCK_H
