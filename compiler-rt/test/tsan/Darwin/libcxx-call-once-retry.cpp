// RUN: %clangxx_tsan -O1 -std=c++11 %s -o %t
// RUN: %env_tsan_opts=ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t safe
// RUN: not %env_tsan_opts=ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t race 2>&1 | FileCheck %s
// CHECK: WARNING: ThreadSanitizer: data race

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>

static std::once_flag flag;
static std::atomic<unsigned> phase{0};
static std::atomic<unsigned> ready{0};
static int payload;
static volatile int unrelated;

static void wait_for(std::atomic<unsigned> &state, unsigned value) {
  // Relaxed polling establishes execution order without publishing payload.
  while (state.load(std::memory_order_relaxed) != value)
    std::this_thread::yield();
}

int main(int argc, char **argv) {
  if (argc != 2)
    std::abort();
  const bool race = std::strcmp(argv[1], "race") == 0;
  if (!race && std::strcmp(argv[1], "safe") != 0)
    std::abort();

  std::thread first([&] {
    try {
      std::call_once(flag, [] {
        payload = 17;
        throw 1;
      });
      std::abort();
    } catch (int value) {
      if (value != 1)
        std::abort();
    }
    phase.store(1, std::memory_order_relaxed);
    wait_for(phase, 2);
    std::call_once(flag, [] { std::abort(); });
    if (payload != 29)
      std::abort();

    if (race) {
      ready.fetch_add(1, std::memory_order_relaxed);
      wait_for(ready, 2);
      unrelated = 1;
    }
  });
  std::thread second([&] {
    wait_for(phase, 1);
    std::call_once(flag, [] {
      // Exceptional active executions precede the next active execution.
      if (payload != 17)
        std::abort();
      payload = 29;
    });
    phase.store(2, std::memory_order_relaxed);

    if (race) {
      ready.fetch_add(1, std::memory_order_relaxed);
      wait_for(ready, 2);
      unrelated = 2;
    }
  });
  first.join();
  second.join();
  return payload == 29 ? 0 : 1;
}
