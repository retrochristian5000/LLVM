# REQUIRES: aarch64

## ARM64e authenticated-pointer relocations must survive all the way through
## Mach-O chained-fixup emission. This is the acceptance test for the first
## ARM64e LLD output path: type 11 must no longer be INVALID, and the linker
## must preserve its pointer-authentication semantics rather than flattening it
## into an ordinary 64-bit relocation.

# RUN: rm -rf %t; split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/foo.o %t/foo.s
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/test.o %t/test.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -install_name @rpath/libfoo.dylib %t/foo.o -o %t/libfoo.dylib
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib %t/libfoo.dylib %t/test.o -o %t/libtest.dylib
# RUN: llvm-objdump --macho --private-header %t/libtest.dylib | \
# RUN:   FileCheck %s --check-prefix=HEADER
# RUN: llvm-objdump --macho --chained-fixups %t/libtest.dylib | \
# RUN:   FileCheck %s --check-prefix=FIXUPS

# HEADER: ARM64          E
# FIXUPS: chained fixups header (LC_DYLD_CHAINED_FIXUPS)
# FIXUPS: pointer_format = 12 (DYLD_CHAINED_PTR_ARM64E_USERLAND24)
# FIXUPS: _foo

#--- foo.s
.text
.globl _foo
_foo:
  ret

#--- test.s
.data
.p2align 3
.quad _foo@AUTH(ia,42,addr)
