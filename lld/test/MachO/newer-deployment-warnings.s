# REQUIRES: x86
# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-macosx14.0 %s -o %t.a.o
# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-macosx14.0 %s -o %t.b.o
# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-macosx15.0 %s -o %t.c.o
# RUN: %no-arg-lld -arch x86_64 -dylib -platform_version macos 12.0 15.0 \
# RUN:   %t.a.o %t.b.o %t.c.o -o %t.dylib 2>&1 | FileCheck %s --check-prefix=ON
# ON: warning: {{.*}} has version 14.0.0, which is newer than target minimum of 12.0
# ON-NOT: has version 14.0.0
# ON: warning: {{.*}} has version 15.0.0, which is newer than target minimum of 12.0
# ON-NOT: newer than target minimum
# RUN: %no-arg-lld -arch x86_64 -dylib -platform_version macos 12.0 15.0 \
# RUN:   -no_warn_newer_deployment -fatal_warnings %t.a.o %t.b.o %t.c.o -o %t.dylib 2>&1 | count 0
# RUN: not %no-arg-lld -arch x86_64 -dylib -platform_version macos 12.0 15.0 \
# RUN:   -no_warn_newer_deployment -warn_newer_deployment -fatal_warnings %t.a.o -o %t.dylib 2>&1 \
# RUN:   | FileCheck %s --check-prefix=FATAL
# FATAL: error: {{.*}} newer than target minimum
# RUN: %no-arg-lld -arch x86_64 -dylib -platform_version macos 15.0 15.0 \
# RUN:   -fatal_warnings %t.a.o %t.c.o -o %t.dylib 2>&1 | count 0
# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-ios15.0-simulator %s -o %t.ios.o
# RUN: not %no-arg-lld -arch x86_64 -dylib -platform_version macos 12.0 15.0 \
# RUN:   -no_warn_newer_deployment %t.ios.o -o %t.dylib 2>&1 | FileCheck %s --check-prefix=PLATFORM
# PLATFORM: error: {{.*}} different from target platform

.text
  ret
