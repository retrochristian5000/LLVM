# REQUIRES: aarch64

## -arch arm64e must survive target selection and be emitted as the
## CPU_SUBTYPE_ARM64E Mach-O subtype rather than being flattened to ARM64_ALL.

# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t.o %s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib %t.o -o %t.dylib
# RUN: llvm-objdump --macho --private-header %t.dylib | FileCheck %s

# CHECK: ARM64          E

.text
.globl _foo
_foo:
  ret
