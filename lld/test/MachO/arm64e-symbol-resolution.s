# REQUIRES: aarch64
#
## Imported branch targets have to enter ARM64e's authenticated stub at its
## first instruction. A symbol+addend branch instead entered the stub midway;
## the symbol's final address is only known at dyld binding time.
## Cover strong, weak and flat-lookup imports, plus valid symbol resolution.
#
# RUN: rm -rf %t; split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/lib.o %t/lib.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -install_name @rpath/libtargets.dylib %t/lib.o \
# RUN:   -o %t/libtargets.dylib
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/strong.o %t/strong.s
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/weak.o %t/weak.s
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/flat.o %t/flat.s
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/valid.o %t/valid.s
#
## A normal import, and a same-image branch to an interior address, are valid.
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -bundle -fixup_chains -undefined dynamic_lookup %t/valid.o \
# RUN:   %t/libtargets.dylib -o %t/valid.bundle
# RUN: llvm-objdump --macho --chained-fixups %t/valid.bundle \
# RUN:   | FileCheck %s --check-prefix=VALID
# VALID: pointer_format = 12 (DYLD_CHAINED_PTR_ARM64E_USERLAND24)
# VALID-DAG: _external
# VALID-DAG: _weak
# VALID-DAG: _host
#
## A dyld-bound symbol+addend cannot resolve to the true function address.
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib %t/strong.o %t/libtargets.dylib -o %t/bad-strong.dylib 2>&1 \
# RUN:   | FileCheck %s --check-prefix=BAD-STRONG
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib %t/weak.o %t/libtargets.dylib -o %t/bad-weak.dylib 2>&1 \
# RUN:   | FileCheck %s --check-prefix=BAD-WEAK
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -bundle -undefined dynamic_lookup %t/flat.o -o %t/bad-flat.bundle 2>&1 \
# RUN:   | FileCheck %s --check-prefix=BAD-FLAT
# BAD-STRONG: BRANCH26 relocation cannot target an offset within dynamically bound symbol _external
# BAD-WEAK: BRANCH26 relocation cannot target an offset within dynamically bound symbol _weak
# BAD-FLAT: BRANCH26 relocation cannot target an offset within dynamically bound symbol _host
#
#--- lib.s
.text
.globl _external, _weak
_external:
  ret
.weak_definition _weak
_weak:
  ret
#
#--- strong.s
.text
.globl _consumer
_consumer:
  bl _external + 4
  ret
#
#--- weak.s
.text
.globl _consumer
_consumer:
  bl _weak + 4
  ret
#
#--- flat.s
.text
.globl _consumer
_consumer:
  bl _host + 4
  ret
#
#--- valid.s
.text
.globl _consumer, _local
_consumer:
  bl _external
  bl _weak
  bl _host
  bl _local + 4
  ret
_local:
  nop
  ret
.data
.p2align 3
.quad _external@AUTH(ia,12,addr)
.quad _weak@AUTH(ib,34)
.quad _host@AUTH(da,56,addr)
.quad _local@AUTH(db,78)
