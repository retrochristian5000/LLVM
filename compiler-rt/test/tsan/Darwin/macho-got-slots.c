// Verify that TSan reports Mach-O indirect symbol-pointer metadata without
// attempting to mutate GOT entries or dyld's authenticated fixups.
//
// RUN: %clang_tsan %s -o %t
// RUN: env TSAN_OPTIONS=verbosity=2 %run %t 2>&1 | FileCheck %s
//
// CHECK: ThreadSanitizer: main Mach-O symbol-pointer slots: lazy={{[0-9]+}}, non-lazy={{[0-9]+}}

#include <stdio.h>

int main(void) {
  puts("Mach-O binding metadata inspected");
  return 0;
}
