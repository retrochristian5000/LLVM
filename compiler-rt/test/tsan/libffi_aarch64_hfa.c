// REQUIRES: tsan-libffi-aarch64
// RUN: %clang_tsan -O1 -pthread %tsan_libffi_cflags %s %tsan_libffi_ldflags -o %t
// RUN: %run %t safe arg-reg
// RUN: not %run %t race arg-reg 2>&1 | FileCheck %s
// RUN: %run %t safe arg-stack
// RUN: not %run %t race arg-stack 2>&1 | FileCheck %s
// RUN: %run %t safe result-float
// RUN: not %run %t race result-float 2>&1 | FileCheck %s
// RUN: %run %t safe result-double
// RUN: not %run %t race result-double 2>&1 | FileCheck %s
// RUN: %run %t adjacent arg-reg
// RUN: %run %t adjacent arg-stack
// RUN: %run %t adjacent result-float
// RUN: %run %t adjacent result-double
// CHECK: WARNING: ThreadSanitizer: data race

#if __has_include(<ffi.h>)
#include <ffi.h>
#else
#include <ffi/ffi.h>
#endif
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

struct Float3 { float a, b, c; };
struct Double3 { double a, b, c; };
static _Alignas(8) struct {
  struct Float3 value;
  volatile float neighbor;
} input = {{1, 2, 3}, 0}, output;
static _Alignas(8) struct {
  struct Double3 value;
  volatile double neighbor;
} double_output;

static ffi_cif cif;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static enum { ArgReg, ArgStack, ResultFloat, ResultDouble } kind;
static enum { Safe, Race, Adjacent } mode;
static volatile float observed;

static void consume(struct Float3 value) { observed = value.c; }
static void consume_stack(double a, double b, double c, double d, double e,
                          double f, double g, double h, struct Float3 value) {
  observed = (float)(a + b + c + d + e + f + g + h) + value.c;
}
static struct Float3 produce_float(void) { return (struct Float3){4, 5, 6}; }
static struct Double3 produce_double(void) { return (struct Double3){7, 8, 9}; }

static void enter(void) {
  if (mode == Safe && pthread_mutex_lock(&lock))
    abort();
}
static void leave(void) {
  if (mode == Safe && pthread_mutex_unlock(&lock))
    abort();
}

static void *call_ffi(void *unused) {
  (void)unused;
  double regs[8] = {1, 1, 1, 1, 1, 1, 1, 1};
  void *values[9];
  enter();
  switch (kind) {
  case ArgReg:
    values[0] = &input.value;
    ffi_call(&cif, FFI_FN(consume), NULL, values);
    break;
  case ArgStack:
    for (unsigned i = 0; i < 8; ++i)
      values[i] = &regs[i];
    values[8] = &input.value;
    ffi_call(&cif, FFI_FN(consume_stack), NULL, values);
    break;
  case ResultFloat:
    ffi_call(&cif, FFI_FN(produce_float), &output.value, NULL);
    break;
  case ResultDouble:
    ffi_call(&cif, FFI_FN(produce_double), &double_output.value, NULL);
    break;
  }
  leave();
  return NULL;
}

static void *compete(void *unused) {
  (void)unused;
  enter();
  switch (kind) {
  case ArgReg:
  case ArgStack:
    if (mode == Adjacent)
      input.neighbor = 23;
    else
      input.value.c = 11;
    break;
  case ResultFloat:
    if (mode == Adjacent)
      output.neighbor = 23;
    else
      output.value.c = 12;
    break;
  case ResultDouble:
    if (mode == Adjacent)
      double_output.neighbor = 23;
    else
      double_output.value.c = 13;
    break;
  }
  leave();
  return NULL;
}

int main(int argc, char **argv) {
  if (argc != 3)
    abort();
  if (!strcmp(argv[1], "safe"))
    mode = Safe;
  else if (!strcmp(argv[1], "race"))
    mode = Race;
  else if (!strcmp(argv[1], "adjacent"))
    mode = Adjacent;
  else
    abort();

  ffi_type *float_members[] = {&ffi_type_float, &ffi_type_float,
                              &ffi_type_float, NULL};
  ffi_type *double_members[] = {&ffi_type_double, &ffi_type_double,
                               &ffi_type_double, NULL};
  ffi_type float3 = {0, 0, FFI_TYPE_STRUCT, float_members};
  ffi_type double3 = {0, 0, FFI_TYPE_STRUCT, double_members};
  ffi_type *types[9];
  ffi_type *ret = &ffi_type_void;
  unsigned count = 0;
  if (!strcmp(argv[2], "arg-reg")) {
    kind = ArgReg;
    types[count++] = &float3;
  } else if (!strcmp(argv[2], "arg-stack")) {
    kind = ArgStack;
    for (unsigned i = 0; i < 8; ++i)
      types[count++] = &ffi_type_double;
    types[count++] = &float3;
  } else if (!strcmp(argv[2], "result-float")) {
    kind = ResultFloat;
    ret = &float3;
  } else if (!strcmp(argv[2], "result-double")) {
    kind = ResultDouble;
    ret = &double3;
  } else {
    abort();
  }
  if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, count, ret,
                   count ? types : NULL) != FFI_OK)
    abort();
  pthread_t caller, writer;
  if (pthread_create(&caller, NULL, call_ffi, NULL) ||
      pthread_create(&writer, NULL, compete, NULL))
    abort();
  if (pthread_join(caller, NULL) || pthread_join(writer, NULL))
    abort();

  // Ensure the real call was performed, not just the modeled access.
  if (kind == ArgReg && observed != 3 && observed != 11)
    abort();
  if (kind == ArgStack && observed != 11 && observed != 19)
    abort();
  if (kind == ResultFloat &&
      (output.value.a != 4 || output.value.b != 5))
    abort();
  if (kind == ResultDouble &&
      (double_output.value.a != 7 || double_output.value.b != 8))
    abort();
  return 0;
}
