# REQUIRES: aarch64
#
## Universal input is a container, not a permission to mix object ABIs.
## Prefer an exact arm64e slice even if an arm64 dylib appears first. Fallback
## from arm64e to arm64 is valid only for dylibs (and bundle-loader images).
## Read both FAT_MAGIC and FAT_MAGIC_64 without weakening object ABI checks.
#
# RUN: rm -rf %t; split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=arm64-apple-macos -o %t/lib-arm64.o %t/lib-arm64.s
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/lib-arm64e.o %t/lib-arm64e.s
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/client-e.o %t/client-e.s
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/client-a.o %t/client-a.s
# RUN: %no-arg-lld -arch arm64 -platform_version macos 13.0 13.0 \
# RUN:   -dylib -install_name @rpath/libfat.dylib %t/lib-arm64.o -o %t/lib-arm64.dylib
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains -install_name @rpath/libfat.dylib \
# RUN:   %t/lib-arm64e.o -o %t/lib-arm64e.dylib
#
## A universal dylib's exact arm64e slice wins even when the arm64 slice is
## available and carries the same install name.
# RUN: llvm-lipo -create %t/lib-arm64.dylib %t/lib-arm64e.dylib -o %t/fat32-both.dylib
# RUN: llvm-lipo -create -fat64 %t/lib-arm64.dylib %t/lib-arm64e.dylib -o %t/fat64-both.dylib
# RUN: llvm-lipo -archs %t/fat64-both.dylib | FileCheck %s --check-prefix=ARCHS
# ARCHS: arm64 arm64e
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/client-e.o %t/fat32-both.dylib -o %t/exact32.dylib
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/client-e.o %t/fat64-both.dylib -o %t/exact64.dylib
# RUN: llvm-objdump --macho --chained-fixups %t/exact64.dylib | FileCheck %s --check-prefix=EXACT
# EXACT: pointer_format = 12 (DYLD_CHAINED_PTR_ARM64E_USERLAND24)
# EXACT: _only_e
#
## There is no exact slice: selecting the arm64 DYLIB is still compatible.
# RUN: llvm-lipo -create %t/lib-arm64.dylib -o %t/fat32-arm64.dylib
# RUN: llvm-lipo -create -fat64 %t/lib-arm64.dylib -o %t/fat64-arm64.dylib
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/client-a.o %t/fat32-arm64.dylib -o %t/fallback32.dylib
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/client-a.o %t/fat64-arm64.dylib -o %t/fallback64.dylib
# RUN: llvm-objdump --macho --chained-fixups %t/fallback64.dylib | FileCheck %s --check-prefix=FALLBACK
# FALLBACK: pointer_format = 12 (DYLD_CHAINED_PTR_ARM64E_USERLAND24)
# FALLBACK: _only_a
#
## Thin or fat arm64 objects remain forbidden from satisfying arm64e symbols.
# RUN: llvm-lipo -create %t/lib-arm64.o -o %t/fat32-arm64.o
# RUN: llvm-lipo -create -fat64 %t/lib-arm64.o -o %t/fat64-arm64.o
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib %t/client-a.o %t/fat32-arm64.o -o %t/reject32.dylib 2>&1 \
# RUN:   | FileCheck %s --check-prefix=NO-OBJECT-FALLBACK
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib %t/client-a.o %t/fat64-arm64.o -o %t/reject64.dylib 2>&1 \
# RUN:   | FileCheck %s --check-prefix=NO-OBJECT-FALLBACK
# NO-OBJECT-FALLBACK: ignoring file because it is universal (arm64) but does not contain the arm64e architecture
#
## A universal arm64e object still links when its actual ABI is version 0.
# RUN: llvm-lipo -create %t/client-e.o -o %t/fat32-arm64e.o
# RUN: llvm-lipo -create -fat64 %t/client-e.o -o %t/fat64-arm64e.o
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/fat32-arm64e.o %t/lib-arm64e.dylib -o %t/object32.dylib
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/fat64-arm64e.o %t/lib-arm64e.dylib -o %t/object64.dylib
#
## A malformed FAT64 descriptor must fail before using its slice bytes.
# RUN: %python -c "import pathlib,sys; p=pathlib.Path(sys.argv[1]); p.write_bytes(pathlib.Path(sys.argv[2]).read_bytes()[:12])" %t/truncated.dylib %t/fat64-both.dylib
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib %t/client-e.o %t/truncated.dylib -o %t/bad.dylib 2>&1 \
# RUN:   | FileCheck %s --check-prefix=BAD-FAT64
# BAD-FAT64: truncated or malformed fat file (fat_arch_64 structs would extend past the end of the file
#
#--- lib-arm64.s
.text
.globl _only_a
_only_a:
  ret
#
#--- lib-arm64e.s
.text
.globl _only_e
_only_e:
  ret
#
#--- client-a.s
.data
.p2align 3
.quad _only_a@AUTH(ia,24,addr)
#
#--- client-e.s
.data
.p2align 3
.quad _only_e@AUTH(ib,42)
