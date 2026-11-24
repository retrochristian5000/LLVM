// REQUIRES: tsan-libffi-runtime
// RUN: %clang_tsan -O1 -pthread %tsan_libffi_cflags %s %tsan_libffi_ldflags -o %t
// RUN: %run %t

#if __has_include(<ffi.h>)
#include <ffi.h>
#else
#include <ffi/ffi.h>
#endif
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

static _Alignas(8) struct {
  uint8_t argument;
  volatile uint8_t neighbor;
} narrow = {7, 0};

static _Alignas(8) struct {
  float result;
  volatile float neighbor;
} floats;

static volatile int pointee;
static volatile int *pointer = &pointee;
static volatile int discarded;
static volatile unsigned observed;

static void consume_byte(uint8_t value) { observed = value; }
static void consume_pointer(volatile int *value) { observed = value != NULL; }
static float produce_float(void) { return 1.25f; }
static void produce_void(void) {}

static void *call_ffi(void *unused) {
  (void)unused;
  ffi_cif cif;
  ffi_type *types[] = {&ffi_type_uint8};
  void *args[] = {&narrow.argument};
  if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_void, types) != FFI_OK)
    abort();
  ffi_call(&cif, FFI_FN(consume_byte), NULL, args);

  types[0] = &ffi_type_pointer;
  args[0] = &pointer;
  if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_void, types) != FFI_OK)
    abort();
  ffi_call(&cif, FFI_FN(consume_pointer), NULL, args);

  if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 0, &ffi_type_float, NULL) != FFI_OK)
    abort();
  ffi_call(&cif, FFI_FN(produce_float), &floats.result, NULL);

  if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 0, &ffi_type_void, NULL) != FFI_OK)
    abort();
  // For a void call the return buffer is ignored by libffi.
  ffi_call(&cif, FFI_FN(produce_void), (void *)&discarded, NULL);
  return NULL;
}

static void *write_neighbors(void *unused) {
  (void)unused;
  narrow.neighbor = 21;
  pointee = 37;
  floats.neighbor = 9.5f;
  discarded = 41;
  return NULL;
}

int main(void) {
  pthread_t caller, writer;
  if (pthread_create(&caller, NULL, call_ffi, NULL) ||
      pthread_create(&writer, NULL, write_neighbors, NULL))
    abort();
  if (pthread_join(caller, NULL) || pthread_join(writer, NULL))
    abort();
  if (floats.result != 1.25f || observed != 1)
    abort();
  return 0;
}
