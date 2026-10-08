# REQUIRES: aarch64

## ARM64e chained fixups use an eight-byte stride. An authenticated pointer
## placed at an unaligned address must fail without committing a Mach-O file.
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t.o %s
# RUN: rm -f %t.dylib
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t.o -o %t.dylib 2>&1 | FileCheck %s
# RUN: test ! -e %t.dylib

# CHECK: ARM64e chained fixup is not 8-byte aligned

.text
_local:
  ret

.data
.long 0
.quad _local@AUTH(ia,42,addr)
