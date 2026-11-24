// Compile the same byte-level oracle with both language frontends.
// RUN: %clang_tsan -O1 -std=c11 %s -o %t.c
// RUN: %clangxx_tsan -O1 -std=c++11 -x c++ %s -o %t.cxx
// RUN: %run %t.c read adjacent
// RUN: %run %t.c write adjacent
// RUN: not %run %t.c read tail 2>&1 | FileCheck %s
// RUN: not %run %t.c write tail 2>&1 | FileCheck %s
// RUN: %run %t.cxx read adjacent
// RUN: %run %t.cxx write adjacent
// RUN: not %run %t.cxx read tail 2>&1 | FileCheck %s
// RUN: not %run %t.cxx write tail 2>&1 | FileCheck %s
// CHECK: WARNING: ThreadSanitizer: data race

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
void __tsan_read16_pc(void *addr, void *pc);
void __tsan_write16_pc(void *addr, void *pc);
#ifdef __cplusplus
}
#endif

static unsigned char bytes[32] __attribute__((aligned(8)));
static int write_access;
static int tail;
static volatile unsigned char observed;

// Explicit instrumentation models an access without performing a second
// compiler-instrumented access to the same storage.
static void *modeled_access(void *unused) {
  (void)unused;
  void *pc = (void *)(uintptr_t)&modeled_access;
  if (write_access)
    __tsan_write16_pc(bytes + 1, pc);
  else
    __tsan_read16_pc(bytes + 1, pc);
  return NULL;
}

static void *competing_access(void *unused) {
  (void)unused;
  // [1,17) spans three shadow cells. Byte 16 is inside, 0 and 17
  // are outside. Use a single-byte volatile access as the independent oracle.
  volatile unsigned char *p = bytes;
  if (write_access) {
    if (tail)
      observed = p[16];
    else {
      observed = p[0];
      observed = p[17];
    }
  } else {
    if (tail)
      p[16] = 7;
    else {
      p[0] = 7;
      p[17] = 9;
    }
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc != 3)
    abort();
  write_access = strcmp(argv[1], "write") == 0;
  if (!write_access && strcmp(argv[1], "read") != 0)
    abort();
  tail = strcmp(argv[2], "tail") == 0;
  if (!tail && strcmp(argv[2], "adjacent") != 0)
    abort();

  pthread_t model, competitor;
  if (pthread_create(&model, NULL, modeled_access, NULL) ||
      pthread_create(&competitor, NULL, competing_access, NULL))
    abort();
  if (pthread_join(model, NULL) || pthread_join(competitor, NULL))
    abort();
  return 0;
}
