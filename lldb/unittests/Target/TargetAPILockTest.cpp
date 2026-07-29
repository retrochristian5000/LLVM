//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "lldb/Target/TargetAPILock.h"
#include "gtest/gtest.h"

#include <mutex>
#include <thread>

using namespace lldb_private;

TEST(TargetAPILockTest, DefaultConstructedIsANoOp) {
  TargetAPILock lock;

  // No synchronization primitive is touched at all in this state, so
  // there is no pairing requirement: try_lock() always succeeds, and
  // lock()/unlock() are callable with no invariant to violate.
  EXPECT_TRUE(lock.try_lock());
  lock.lock();
  lock.unlock();
  lock.lock();
  lock.unlock();
}

TEST(TargetAPILockTest, WrapsARealMutex) {
  std::recursive_mutex mutex;
  TargetAPILock lock(mutex);

  lock.lock();

  // Recursive reentrancy is delegated straight to the underlying
  // std::recursive_mutex: a second handle over the same mutex, locked
  // from the same thread, must not block.
  TargetAPILock second_lock(mutex);
  EXPECT_TRUE(second_lock.try_lock());
  second_lock.unlock();

  lock.unlock();

  // Once fully unlocked (both handles released), a background thread
  // must be able to acquire the same underlying mutex.
  std::thread t([&mutex]() {
    TargetAPILock background_lock(mutex);
    EXPECT_TRUE(background_lock.try_lock());
    background_lock.unlock();
  });
  t.join();
}

TEST(TargetAPILockTest, RealMutexBlocksOtherThreads) {
  std::recursive_mutex mutex;
  TargetAPILock lock(mutex);

  lock.lock();

  // While held on this thread, a different thread must not be able to
  // acquire the same underlying mutex.
  std::thread t([&mutex]() {
    TargetAPILock background_lock(mutex);
    EXPECT_FALSE(background_lock.try_lock());
  });
  t.join();

  lock.unlock();
}
