# REQUIRES: aarch64

## ARM64e authenticated-pointer relocations must be recognized as a real
## relocation kind rather than falling through to INVALID relocation metadata.
## The first porting slice intentionally stops before chained-fixup emission;
## keep that boundary explicit so later work can turn this test into a success
## case without ever accepting a silently malformed pointer.

# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t.o %s
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib %t.o -o %t.dylib 2>&1 | FileCheck %s

# CHECK: error: ARM64e authenticated pointer relocation output is not implemented yet
# CHECK-NOT: INVALID relocation has invalid width

.text
.globl _foo
_foo:
  ret

.data
.p2align 3
.quad _foo@AUTH(ia,42,addr)
