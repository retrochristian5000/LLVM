// REQUIRES: tsan-objcxx-runtime
// RUN: %clangxx_tsan -O1 -fno-objc-arc %s -lobjc -o %t
// RUN: %env_tsan_opts=verbosity=2:ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t safe 2>&1 | FileCheck %s --check-prefix=SAFE
// RUN: not %env_tsan_opts=ignore_noninstrumented_modules=0:ignore_interceptors_accesses=0 %run %t race 2>&1 | FileCheck %s --check-prefix=RACE
// SAFE: ThreadSanitizer: Objective-C image placement: supported
// SAFE-NOT: WARNING: ThreadSanitizer
// SAFE: Objective-C relocated ivar: verified
// SAFE-NOT: WARNING: ThreadSanitizer
// RACE: WARNING: ThreadSanitizer: data race

#include <objc/runtime.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" void objc_copyStruct(void *, const void *, ptrdiff_t, BOOL, BOOL);

__attribute__((objc_root_class))
@interface TSanRelocatedObject {
@public
  Class isa;
  unsigned char payload[3];
  volatile unsigned char neighbor;
}
@end

@implementation TSanRelocatedObject
@end

static TSanRelocatedObject *object;
static bool race;

static void *read_ivar(void *) {
  unsigned char copy[3];
  objc_copyStruct(copy, object->payload, sizeof(copy), NO, NO);
  if (copy[0] != 1 || copy[1] != 2)
    abort();
  return nullptr;
}

static void *write_ivar(void *) {
  if (race)
    static_cast<volatile unsigned char &>(object->payload[2]) = 7;
  else
    object->neighbor = 9;
  return nullptr;
}

int main(int argc, char **argv) {
  if (argc != 2)
    abort();
  race = strcmp(argv[1], "race") == 0;
  if (!race && strcmp(argv[1], "safe") != 0)
    abort();

  Class cls = objc_getClass("TSanRelocatedObject");
  if (!cls || !class_getImageName(cls))
    abort();
  object = (TSanRelocatedObject *)class_createInstance(cls, 0);
  if (!object)
    abort();

  Ivar ivar = class_getInstanceVariable(cls, "payload");
  if (!ivar)
    abort();
  const ptrdiff_t offset = ivar_getOffset(ivar);
  const size_t size = class_getInstanceSize(cls);
  if (offset < 0 || static_cast<size_t>(offset) > size ||
      sizeof(object->payload) > size - static_cast<size_t>(offset))
    abort();
  // Use the runtime's resolved ivar offset, never a preferred Mach-O address.
  if (reinterpret_cast<unsigned char *>(object) + offset != object->payload)
    abort();
  object->payload[0] = 1;
  object->payload[1] = 2;

  pthread_t reader, writer;
  if (pthread_create(&reader, nullptr, read_ivar, nullptr) ||
      pthread_create(&writer, nullptr, write_ivar, nullptr))
    abort();
  if (pthread_join(reader, nullptr) || pthread_join(writer, nullptr))
    abort();
  object_dispose(object);
  fprintf(stderr, "Objective-C relocated ivar: verified\n");
  return 0;
}
