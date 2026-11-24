// RUN: %clang_tsan -O1 -std=c11 -D_DARWIN_C_SOURCE %s %S/../../../lib/tsan/mman/tsan_mman_range_posix.c -o %t
// RUN: %run %t

#include "../../../lib/tsan/mman/tsan_mman_range_posix.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

static void check(uintptr_t addr, size_t size, size_t page,
                  uintptr_t expected_begin, size_t expected_length) {
  uintptr_t begin = 0;
  size_t length = 0;
  if (!__tsan_posix_page_extent(addr, size, page, &begin, &length) ||
      begin != expected_begin || length != expected_length)
    abort();
}

static void reject(uintptr_t addr, size_t size, size_t page) {
  uintptr_t begin = 123;
  size_t length = 456;
  if (__tsan_posix_page_extent(addr, size, page, &begin, &length) ||
      begin != 123 || length != 456)
    abort();
}

int main(void) {
  const long native_page = sysconf(_SC_PAGESIZE);
  if (native_page <= 0)
    abort();
  const size_t page = (size_t)native_page;
  for (size_t p = 4096; p <= 16384; p *= 4) {
    check(p, 1, p, p, p);
    check(p, p, p, p, p);
    check(p, p + 1, p, p, 2 * p);
    check(p + p - 1, 2, p, p, 2 * p);
    check((uintptr_t)1 << 40, 1, p, (uintptr_t)1 << 40, p);
    reject(UINTPTR_MAX - 1, 4, p);
    reject(UINTPTR_MAX - 1, 1, p);
    reject(p, 0, p);
    reject(p, 1, 0);
    reject(p, 1, 3);
  }
  void *mapping = mmap(NULL, 1, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANON, -1, 0);
  if (mapping == MAP_FAILED)
    abort();
  check((uintptr_t)mapping, 1, page, (uintptr_t)mapping, page);
  if (munmap(mapping, 1) != 0)
    abort();
  return 0;
}
