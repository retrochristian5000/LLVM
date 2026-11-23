// Verify that verbose TSan startup identifies the main Mach-O executable.
// The check describes the executable image, not the TSan runtime dylib.
//
// RUN: %clang_tsan %s -o %t
// RUN: env TSAN_OPTIONS=verbosity=2 %run %t 2>&1 | FileCheck %s
//
// CHECK: ThreadSanitizer: main Mach-O executable architecture: {{(x86_64|arm64e|arm64)$}}

int main(void) { return 0; }
