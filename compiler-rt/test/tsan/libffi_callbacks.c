// REQUIRES: tsan-libffi
// RUN: %clang_tsan -O1 -pthread %tsan_libffi_cflags %s %tsan_libffi_ldflags -o %t
// RUN: %run %t abi
// RUN: %run %t direct-safe
// RUN: %run %t ffi-safe
// RUN: not %run %t direct-race 2>&1 | FileCheck %s --check-prefix=RACE
// RUN: not %run %t ffi-race 2>&1 | FileCheck %s --check-prefix=RACE
//
// RACE: WARNING: ThreadSanitizer: data race

#if __has_include(<ffi.h>)
#include <ffi.h>
#else
#include <ffi/ffi.h>
#endif
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(int ok, const char *message) {
  if (!ok) {
    fprintf(stderr, "libffi TSan fixture: %s\n", message);
    exit(1);
  }
}

static uint64_t mixed(uint64_t a, double b) {
  return a + (uint64_t)b;
}

static double variadic(unsigned count, ...) {
  va_list ap;
  va_start(ap, count);
  double sum = 0;
  for (unsigned i = 0; i < count; ++i)
    sum += va_arg(ap, double);
  va_end(ap);
  return sum;
}

struct Pair {
  uint64_t integer;
  double real;
};

static struct Pair pair_increment(struct Pair p) {
  p.integer += 7;
  p.real += 0.5;
  return p;
}

static void check_abi(void) {
  ffi_cif cif;
  uint64_t a = 37, result = 0;
  double b = 5.0;
  ffi_type *mixed_types[] = {&ffi_type_uint64, &ffi_type_double};
  void *mixed_args[] = {&a, &b};
  require(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 2, &ffi_type_uint64,
                       mixed_types) == FFI_OK,
          "cannot prepare mixed integer/floating-point call");
  ffi_call(&cif, FFI_FN(mixed), &result, mixed_args);
  require(result == mixed(a, b), "mixed call returned incorrect value");

  unsigned count = 2;
  double x = 1.25, y = 2.5, sum = 0;
  ffi_type *var_types[] = {&ffi_type_uint, &ffi_type_double, &ffi_type_double};
  void *var_args[] = {&count, &x, &y};
  require(ffi_prep_cif_var(&cif, FFI_DEFAULT_ABI, 1, 3, &ffi_type_double,
                           var_types) == FFI_OK,
          "cannot prepare variadic call");
  ffi_call(&cif, FFI_FN(variadic), &sum, var_args);
  require(sum == variadic(count, x, y), "variadic call returned incorrect value");

  ffi_type *elements[] = {&ffi_type_uint64, &ffi_type_double, NULL};
  ffi_type pair_type = {0, 0, FFI_TYPE_STRUCT, elements};
  ffi_type *pair_types[] = {&pair_type};
  struct Pair input = {11, 2.5}, output = {0, 0};
  void *pair_args[] = {&input};
  require(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &pair_type, pair_types) ==
              FFI_OK,
          "cannot prepare aggregate call");
  ffi_call(&cif, FFI_FN(pair_increment), &output, pair_args);
  struct Pair expected = pair_increment(input);
  require(output.integer == expected.integer && output.real == expected.real,
          "aggregate call returned incorrect value");
}

typedef void (*Callback)(uint64_t);
static volatile uint64_t shared_value;
static pthread_mutex_t start_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t value_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned arrived;

static void write_value(uint64_t value) { shared_value = value; }

static void closure_callback(ffi_cif *cif, void *ret, void **args,
                             void *context) {
  (void)cif;
  (void)ret;
  (void)context;
  write_value(*(uint64_t *)args[0]);
}

struct Worker {
  Callback callback;
  uint64_t value;
  int synchronized;
};

static void *worker(void *opaque) {
  struct Worker *w = opaque;
  require(pthread_mutex_lock(&start_lock) == 0, "start lock failed");
  if (++arrived == 2)
    require(pthread_cond_broadcast(&start_cond) == 0, "broadcast failed");
  while (arrived != 2)
    require(pthread_cond_wait(&start_cond, &start_lock) == 0, "wait failed");
  require(pthread_mutex_unlock(&start_lock) == 0, "start unlock failed");

  if (w->synchronized)
    require(pthread_mutex_lock(&value_lock) == 0, "value lock failed");
  w->callback(w->value);
  if (w->synchronized)
    require(pthread_mutex_unlock(&value_lock) == 0, "value unlock failed");
  return NULL;
}

static void check_callbacks(int use_ffi, int synchronized) {
  ffi_cif cif;
  ffi_type *types[] = {&ffi_type_uint64};
  ffi_closure *closure = NULL;
  void *code = NULL;
  Callback callback = write_value;
  if (use_ffi) {
    require(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_void, types) ==
                FFI_OK,
            "cannot prepare closure signature");
    closure = ffi_closure_alloc(sizeof(*closure), &code);
    require(closure != NULL && code != NULL, "closure allocation failed");
    require(ffi_prep_closure_loc(closure, &cif, closure_callback, NULL, code) ==
                FFI_OK,
            "closure preparation failed");
    // Use libffi's callable address without modifying authentication bits.
    callback = (Callback)code;
  }

  pthread_t threads[2];
  struct Worker workers[] = {{callback, 17, synchronized},
                             {callback, 29, synchronized}};
  for (unsigned i = 0; i < 2; ++i)
    require(pthread_create(&threads[i], NULL, worker, &workers[i]) == 0,
            "thread creation failed");
  for (unsigned i = 0; i < 2; ++i)
    require(pthread_join(threads[i], NULL) == 0, "thread join failed");
  if (closure != NULL)
    ffi_closure_free(closure);
  require(shared_value == 17 || shared_value == 29, "callback did not run");
}

int main(int argc, char **argv) {
  require(argc == 2, "expected one test mode");
  if (strcmp(argv[1], "abi") == 0)
    check_abi();
  else if (strcmp(argv[1], "direct-safe") == 0)
    check_callbacks(0, 1);
  else if (strcmp(argv[1], "ffi-safe") == 0)
    check_callbacks(1, 1);
  else if (strcmp(argv[1], "direct-race") == 0)
    check_callbacks(0, 0);
  else if (strcmp(argv[1], "ffi-race") == 0)
    check_callbacks(1, 0);
  else
    require(0, "unknown test mode");
  return 0;
}
