// REQUIRES: aarch64-target-arch, native-run, system-darwin
// RUN: %clang_builtins %s %librt -o %t && %run %t

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/sysctl.h>

static int modern_error, modern_value, legacy_error, legacy_value;
static size_t modern_size, legacy_size;
static unsigned calls;

static int mock_sysctlbyname(const char *name, void *out, size_t *size,
                            void *newp, size_t newlen) {
  assert(!newp && !newlen && *size == sizeof(int));
  const int modern = !strcmp(name, "hw.optional.arm.FEAT_LSE");
  assert(modern || !strcmp(name, "hw.optional.armv8_1_atomics"));
  ++calls;
  int error = modern ? modern_error : legacy_error;
  if (error) {
    errno = error;
    return -1;
  }
  *(int *)out = modern ? modern_value : legacy_value;
  *size = modern ? modern_size : legacy_size;
  return 0;
}

static _Bool __aarch64_have_lse_atomics;
#define false 0
#define CONSTRUCTOR_ATTRIBUTE
#define sysctlbyname mock_sysctlbyname
#include "cpu_model/aarch64/lse_atomics/apple.inc"
#undef sysctlbyname

static void check(int me, int mv, size_t ms, int le, int lv, size_t ls,
                  int expected, unsigned expected_calls) {
  modern_error = me;
  modern_value = mv;
  modern_size = ms;
  legacy_error = le;
  legacy_value = lv;
  legacy_size = ls;
  calls = 0;
  errno = EBUSY;
  init_have_lse_atomics();
  assert(__aarch64_have_lse_atomics == expected);
  assert(calls == expected_calls);
  assert(errno == EBUSY);
}

int main(void) {
  const size_t n = sizeof(int);
  check(0, 1, n, 0, 0, n, 1, 1);
  check(0, 0, n, 0, 1, n, 0, 1);
  check(0, 1, 1, 0, 1, n, 0, 1);
  check(0, 2, n, 0, 1, n, 0, 1);
  check(ENOENT, 0, n, 0, 1, n, 1, 2);
  check(ENOENT, 0, n, 0, 0, n, 0, 2);
  check(ENOENT, 0, n, ENOENT, 0, n, 0, 2);
  check(ENOENT, 0, n, 0, 1, 1, 0, 2);
  check(EPERM, 0, n, 0, 1, n, 0, 1);
  return 0;
}
