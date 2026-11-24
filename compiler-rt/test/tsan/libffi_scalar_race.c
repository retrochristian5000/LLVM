// REQUIRES: tsan-libffi-runtime
// RUN: %clang_tsan -O1 -pthread %tsan_libffi_cflags %s %tsan_libffi_ldflags -o %t
// RUN: %run %t safe narrow
// RUN: not %run %t race narrow 2>&1 | FileCheck %s --check-prefix=READ
// RUN: %run %t safe float
// RUN: not %run %t race float 2>&1 | FileCheck %s --check-prefix=READ
// RUN: %run %t safe slot
// RUN: not %run %t race slot 2>&1 | FileCheck %s --check-prefix=READ
// RUN: %run %t safe result
// RUN: not %run %t race result 2>&1 | FileCheck %s --check-prefix=WRITE
// RUN: %run %t safe small-result
// RUN: not %run %t race small-result 2>&1 | FileCheck %s --check-prefix=WRITE
// RUN: %run %t safe float-result
// RUN: not %run %t race float-result 2>&1 | FileCheck %s --check-prefix=WRITE
// READ: WARNING: ThreadSanitizer: data race
// WRITE: WARNING: ThreadSanitizer: data race

#if __has_include(<ffi.h>)
#include <ffi.h>
#else
#include <ffi/ffi.h>
#endif
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static ffi_cif cif;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int safe;
static enum { Narrow, Float, Slot, Result, SmallResult, FloatResult } kind;
static uint8_t narrow;
static float real;
static uint64_t slot_a = 11, slot_b = 29, result;
static ffi_arg small_result;
static float float_result;
static void *slot[] = {&slot_a};
static volatile uint64_t observed;

static void consume_narrow(uint8_t value) { observed = value; }
static void consume_float(float value) { observed = (uint64_t)value; }
static void consume_slot(uint64_t value) { observed = value; }
static uint64_t produce(void) { return 17; }
static uint8_t produce_small(void) { return 23; }
static float produce_float(void) { return 3.5f; }

static void enter(void) {
  if (safe && pthread_mutex_lock(&lock))
    abort();
}

static void leave(void) {
  if (safe && pthread_mutex_unlock(&lock))
    abort();
}

static void *call(void *unused) {
  (void)unused;
  void *narrow_args[] = {&narrow};
  void *float_args[] = {&real};
  enter();
  switch (kind) {
  case Narrow:
    ffi_call(&cif, FFI_FN(consume_narrow), NULL, narrow_args);
    break;
  case Float:
    ffi_call(&cif, FFI_FN(consume_float), NULL, float_args);
    break;
  case Slot:
    ffi_call(&cif, FFI_FN(consume_slot), NULL, slot);
    break;
  case Result:
    ffi_call(&cif, FFI_FN(produce), &result, NULL);
    break;
  case SmallResult:
    ffi_call(&cif, FFI_FN(produce_small), &small_result, NULL);
    break;
  case FloatResult:
    ffi_call(&cif, FFI_FN(produce_float), &float_result, NULL);
    break;
  }
  leave();
  return NULL;
}

static void *competing_access(void *unused) {
  (void)unused;
  enter();
  switch (kind) {
  case Narrow:
    narrow = 7;
    break;
  case Float:
    real = 9.0f;
    break;
  case Slot:
    slot[0] = &slot_b;
    break;
  case Result:
    result = 31;
    break;
  case SmallResult:
    small_result = 37;
    break;
  case FloatResult:
    float_result = 4.5f;
    break;
  }
  leave();
  return NULL;
}

int main(int argc, char **argv) {
  if (argc != 3)
    abort();
  safe = strcmp(argv[1], "safe") == 0;
  if (!safe && strcmp(argv[1], "race") != 0)
    abort();
  ffi_type *types[] = {NULL};
  ffi_type *ret = &ffi_type_void;
  if (strcmp(argv[2], "narrow") == 0) {
    kind = Narrow;
    types[0] = &ffi_type_uint8;
  } else if (strcmp(argv[2], "float") == 0) {
    kind = Float;
    types[0] = &ffi_type_float;
  } else if (strcmp(argv[2], "slot") == 0) {
    kind = Slot;
    types[0] = &ffi_type_uint64;
  } else if (strcmp(argv[2], "result") == 0) {
    kind = Result;
    ret = &ffi_type_uint64;
  } else if (strcmp(argv[2], "small-result") == 0) {
    kind = SmallResult;
    ret = &ffi_type_uint8;
  } else if (strcmp(argv[2], "float-result") == 0) {
    kind = FloatResult;
    ret = &ffi_type_float;
  } else {
    abort();
  }
  if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, types[0] ? 1 : 0, ret,
                   types[0] ? types : NULL) != FFI_OK)
    abort();
  pthread_t caller, competitor;
  if (pthread_create(&caller, NULL, call, NULL) ||
      pthread_create(&competitor, NULL, competing_access, NULL))
    abort();
  if (pthread_join(caller, NULL) || pthread_join(competitor, NULL))
    abort();
  return 0;
}
