# REQUIRES: aarch64

## ARM64e imported dylib pointers must retain authenticated bindings while
## selecting 32- vs 64-bit out-of-line addends from the signed 32-bit limit.

# RUN: rm -rf %t; split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/producer.o %t/producer.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -install_name @rpath/libimport-test.dylib %t/producer.o -o %t/libimport-test.dylib

# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/pos.o --defsym ADDEND=8388608 %t/consumer.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/pos.o %t/libimport-test.dylib -o %t/pos.dylib
# RUN: llvm-objdump --macho --chained-fixups %t/pos.dylib | FileCheck %s --check-prefixes=FMT32,COMMON

# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/neg.o --defsym ADDEND=-8388609 %t/consumer.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/neg.o %t/libimport-test.dylib -o %t/neg.dylib
# RUN: llvm-objdump --macho --chained-fixups %t/neg.dylib | FileCheck %s --check-prefixes=FMT32,COMMON

# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/max.o --defsym ADDEND=2147483647 %t/consumer.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/max.o %t/libimport-test.dylib -o %t/max.dylib
# RUN: llvm-objdump --macho --chained-fixups %t/max.dylib | FileCheck %s --check-prefixes=FMT32,COMMON

# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/min.o --defsym ADDEND=-2147483648 %t/consumer.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/min.o %t/libimport-test.dylib -o %t/min.dylib
# RUN: llvm-objdump --macho --chained-fixups %t/min.dylib | FileCheck %s --check-prefixes=FMT32,COMMON

# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/big.o --defsym ADDEND=2147483648 %t/consumer.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/big.o %t/libimport-test.dylib -o %t/big.dylib
# RUN: llvm-objdump --macho --chained-fixups %t/big.dylib | FileCheck %s --check-prefixes=FMT64,COMMON

# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/small.o --defsym ADDEND=-2147483649 %t/consumer.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/small.o %t/libimport-test.dylib -o %t/small.dylib
# RUN: llvm-objdump --macho --chained-fixups %t/small.dylib | FileCheck %s --check-prefixes=FMT64,COMMON

# FMT32: imports_format = 2 (DYLD_CHAINED_IMPORT_ADDEND)
# FMT64: imports_format = 3 (DYLD_CHAINED_IMPORT_ADDEND64)
# COMMON: pointer_format = 12 (DYLD_CHAINED_PTR_ARM64E_USERLAND24)
# COMMON: _dysym

#--- producer.s
.text
.globl _dysym
_dysym:
  ret

#--- consumer.s
.data
.p2align 3
.quad _dysym + ADDEND
.quad _dysym@AUTH(ia,42,addr)
