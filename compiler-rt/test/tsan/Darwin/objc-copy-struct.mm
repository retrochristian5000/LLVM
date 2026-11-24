// REQUIRES: tsan-objcxx-runtime
// RUN: %clangxx_tsan -O1 -fno-objc-arc %s -lobjc -o %t
// RUN: %env_tsan_opts=ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t safe
// RUN: %env_tsan_opts=ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t adjacent
// RUN: not %env_tsan_opts=ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t source 2>&1 | FileCheck %s
// RUN: not %env_tsan_opts=ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t destination 2>&1 | FileCheck %s
// CHECK: WARNING: ThreadSanitizer: data race

#include <objc/objc.h>
#include <stddef.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

extern "C" void objc_copyStruct(void *, const void *, ptrdiff_t, BOOL, BOOL);

struct Payload {
  unsigned char bytes[3];
};
struct alignas(8) Storage {
  Payload value;
  volatile unsigned char neighbor;
};
static Storage source = {{1, 2, 3}, 0}, destination;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static enum { Safe, Adjacent, Source, Destination } mode;

static void *copy_payload(void *) {
  if (mode == Safe && pthread_mutex_lock(&lock))
    abort();
  objc_copyStruct(&destination.value, &source.value,
                  sizeof(Payload), NO, NO);
  if (mode == Safe && pthread_mutex_unlock(&lock))
    abort();
  return nullptr;
}

static void *compete(void *) {
  if (mode == Safe && pthread_mutex_lock(&lock))
    abort();
  if (mode == Adjacent) {
    source.neighbor = 7;
    destination.neighbor = 8;
  } else if (mode == Destination) {
    static_cast<volatile unsigned char &>(destination.value.bytes[2]) = 9;
  } else {
    static_cast<volatile unsigned char &>(source.value.bytes[2]) = 9;
  }
  if (mode == Safe && pthread_mutex_unlock(&lock))
    abort();
  return nullptr;
}

int main(int argc, char **argv) {
  if (argc != 2 || !@encode(BOOL)[0])
    abort();
  if (!strcmp(argv[1], "safe"))
    mode = Safe;
  else if (!strcmp(argv[1], "adjacent"))
    mode = Adjacent;
  else if (!strcmp(argv[1], "source"))
    mode = Source;
  else if (!strcmp(argv[1], "destination"))
    mode = Destination;
  else
    abort();

  pthread_t copier, writer;
  if (pthread_create(&copier, nullptr, copy_payload, nullptr) ||
      pthread_create(&writer, nullptr, compete, nullptr))
    abort();
  if (pthread_join(copier, nullptr) || pthread_join(writer, nullptr))
    abort();
  if (destination.value.bytes[0] != 1 || destination.value.bytes[1] != 2)
    abort();
  return 0;
}
