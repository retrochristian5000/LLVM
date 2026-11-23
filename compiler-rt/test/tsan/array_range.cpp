// RUN: %clangxx_tsan -O1 %s -o %t && %deflake %run %t 2>&1 | FileCheck %s --check-prefix=RACE
// RUN: %clangxx_tsan -O1 -DARRAY_DISJOINT %s -o %t.disjoint && %run %t.disjoint 2>&1 | FileCheck %s --check-prefix=DISJOINT
//
// Adjacent byte-array writes must not be mistaken for a race, even when they
// share a shadow-memory cell. An overlapping write must still be detected.

#include "test.h"

char array[32];

void *WriteUpperHalf(void *) {
  barrier_wait(&barrier);
  for (int i = 4; i < 8; ++i)
    array[i] = 1;
  return nullptr;
}

int main() {
  barrier_init(&barrier, 2);
  pthread_t thread;
  pthread_create(&thread, nullptr, WriteUpperHalf, nullptr);
  barrier_wait(&barrier);
  for (int i = 0; i < 4; ++i)
    array[i] = 2;
#ifndef ARRAY_DISJOINT
  array[6] = 3;
#endif
  pthread_join(thread, nullptr);
  fprintf(stderr, "DONE\n");
  return 0;
}

// RACE: WARNING: ThreadSanitizer: data race
// RACE: DONE
// DISJOINT-NOT: WARNING: ThreadSanitizer:
// DISJOINT: DONE
