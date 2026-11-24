// REQUIRES: tsan-libffi-runtime
// RUN: %clang_tsan -O1 -pthread %tsan_libffi_cflags %s %tsan_libffi_ldflags -o %t
// RUN: %run %t safe
// RUN: not %run %t race 2>&1 | FileCheck %s
// CHECK: WARNING: ThreadSanitizer: data race

#if __has_include(<ffi.h>)
#include <ffi.h>
#else
#include <ffi/ffi.h>
#endif
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint64_t argument;
static volatile uint64_t observed;
static ffi_cif cif;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int safe;

static void consume(uint64_t value) { observed = value; }

static void *call(void *unused) {
  (void)unused;
  void *values[] = {&argument};
  if (safe)
    pthread_mutex_lock(&mutex);
  ffi_call(&cif, FFI_FN(consume), NULL, values);
  if (safe)
    pthread_mutex_unlock(&mutex);
  return NULL;
}

static void *write_argument(void *unused) {
  (void)unused;
  if (safe)
    pthread_mutex_lock(&mutex);
  argument = 42;
  if (safe)
    pthread_mutex_unlock(&mutex);
  return NULL;
}

int main(int argc, char **argv) {
  if (argc != 2)
    abort();
  safe = strcmp(argv[1], "safe") == 0;
  if (!safe && strcmp(argv[1], "race") != 0)
    abort();
  ffi_type *types[] = {&ffi_type_uint64};
  if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_void, types) != FFI_OK)
    abort();
  pthread_t caller, writer;
  if (pthread_create(&caller, NULL, call, NULL) ||
      pthread_create(&writer, NULL, write_argument, NULL))
    abort();
  if (pthread_join(caller, NULL) || pthread_join(writer, NULL))
    abort();
  return 0;
}
