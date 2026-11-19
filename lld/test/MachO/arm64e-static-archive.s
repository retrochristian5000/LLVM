# REQUIRES: aarch64
#
## A Darwin static archive must preserve ARM64e object subtypes and
## authenticated-pointer relocations across llvm-ar and llvm-ranlib.
## ABI checks apply to extracted members, not unused lazy members.
## A universal archive's arm64 slice must never stand in for arm64e objects.

# RUN: rm -rf %t; split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/auth.o %t/auth.s
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/client.o %t/client.s
# RUN: llvm-mc -filetype=obj -triple=arm64-apple-macos -o %t/plain.o %t/plain.s
# RUN: llvm-objdump -r %t/auth.o | FileCheck %s --check-prefix=AUTH-RELOC
# AUTH-RELOC: ARM64_RELOC_AUTHENTICATED_POINTER _from_archive

## Exercise the archiver's automatic Darwin-format selection and rebuilt index.
# RUN: llvm-ar rcs %t/libauth.a %t/auth.o
# RUN: llvm-ranlib %t/libauth.a
# RUN: llvm-ar rcs %t/libplain.a %t/plain.o
# RUN: llvm-ranlib %t/libplain.a
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/client.o %t/libauth.a -o %t/auth.dylib
# RUN: llvm-readobj --file-headers %t/auth.dylib | FileCheck %s --check-prefix=ARM64E
# RUN: llvm-objdump --macho --chained-fixups %t/auth.dylib | FileCheck %s --check-prefix=FIXUPS
# RUN: llvm-nm %t/auth.dylib | FileCheck %s --check-prefix=SYMS
# ARM64E: CpuSubType: CPU_SUBTYPE_ARM64E (0x2)
# ARM64E-NEXT: CpuSubTypeCapabilities: 0x80000000
# ARM64E-NEXT: PointerAuthABI: Userland
# ARM64E-NEXT: PointerAuthABIVersion: 0
# FIXUPS: pointer_format = 12 (DYLD_CHAINED_PTR_ARM64E_USERLAND24)
# SYMS: T _from_archive

## Unused incompatible static archives are harmless. When a member is needed,
## ARM64e/ARM64 mismatches must fail instead of losing the PAC ABI.
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/client.o %t/libplain.a %t/libauth.a \
# RUN:   -o %t/unused-plain.dylib
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -u _plain %t/libplain.a -o %t/bad-plain.dylib 2>&1 \
# RUN:   | FileCheck %s --check-prefix=BAD-E
# BAD-E: unsupported arm64e pointer-authentication ABI
# RUN: not %no-arg-lld -arch arm64 -platform_version macos 13.0 13.0 \
# RUN:   -dylib -u _from_archive %t/libauth.a -o %t/bad-e.dylib 2>&1 \
# RUN:   | FileCheck %s --check-prefix=BAD-A
# BAD-A: arm64e object is incompatible with an arm64 output

## A versioned ARM64e object from a different PAC ABI must not be silently
## relabelled by archive indexing or ARM64e output generation.
# RUN: %python -c "import struct,sys; b=bytearray(open(sys.argv[1],'rb').read()); struct.pack_into('<I',b,8,0x81000002); open(sys.argv[2],'wb').write(b)" %t/auth.o %t/v1.o
# RUN: llvm-ar rcs %t/libv1.a %t/v1.o
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/client.o %t/libauth.a %t/libv1.a \
# RUN:   -o %t/unused-v1.dylib
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -u _from_archive %t/libv1.a -o %t/bad-v1.dylib 2>&1 \
# RUN:   | FileCheck %s --check-prefix=BAD-E

## A fat static archive must select the exact architecture and retain its
## original member objects, not flatten the two ARM64 ABIs together.
# RUN: llvm-lipo -create %t/libplain.a %t/libauth.a -o %t/libfat.a
# RUN: llvm-lipo -archs %t/libfat.a | FileCheck %s --check-prefix=ARCHS
# RUN: llvm-lipo -thin arm64e %t/libfat.a -o %t/thin-auth.a
# RUN: llvm-lipo -thin arm64 %t/libfat.a -o %t/thin-plain.a
# RUN: cmp %t/thin-auth.a %t/libauth.a
# RUN: cmp %t/thin-plain.a %t/libplain.a
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/client.o %t/libfat.a -o %t/fat-e.dylib
# RUN: llvm-objdump --macho --chained-fixups %t/fat-e.dylib | FileCheck %s --check-prefix=FIXUPS
# RUN: %no-arg-lld -arch arm64 -platform_version macos 13.0 13.0 \
# RUN:   -dylib -u _plain %t/libfat.a -o %t/fat-a.dylib
# RUN: llvm-readobj --file-headers %t/fat-a.dylib | FileCheck %s --check-prefix=ARM64
## FAT64 static archives must have the same lazy-member semantics and ABI
## isolation as FAT32. llvm-lipo builds the outer container; llvm-ar and
## llvm-ranlib index each ordinary archive independently.
# RUN: llvm-lipo -create -fat64 %t/libplain.a %t/libauth.a -o %t/libfat64.a
# RUN: llvm-lipo -archs %t/libfat64.a | FileCheck %s --check-prefix=ARCHS
# RUN: llvm-lipo -thin arm64e %t/libfat64.a -o %t/thin64-auth.a
# RUN: llvm-lipo -thin arm64 %t/libfat64.a -o %t/thin64-plain.a
# RUN: cmp %t/thin64-auth.a %t/libauth.a
# RUN: cmp %t/thin64-plain.a %t/libplain.a
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/client.o %t/libfat64.a -o %t/fat64-e.dylib
# RUN: llvm-readobj --file-headers %t/fat64-e.dylib | FileCheck %s --check-prefix=ARM64E
# RUN: llvm-objdump --macho --chained-fixups %t/fat64-e.dylib | FileCheck %s --check-prefix=FIXUPS
# RUN: %no-arg-lld -arch arm64 -platform_version macos 13.0 13.0 \
# RUN:   -dylib -u _plain %t/libfat64.a -o %t/fat64-a.dylib
# RUN: llvm-readobj --file-headers %t/fat64-a.dylib | FileCheck %s --check-prefix=ARM64

## No ARM64e slice: do not allow the plain archive to satisfy an ARM64e
## reference even when the ARM64 archive has the required symbol.
# RUN: llvm-lipo -create %t/libplain.a -o %t/only-plain32.a
# RUN: llvm-lipo -create -fat64 %t/libplain.a -o %t/only-plain64.a
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -u _plain %t/only-plain32.a -o %t/no-fallback32.dylib 2>&1 \
# RUN:   | FileCheck %s --check-prefix=NO-ARCHIVE-FALLBACK
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -u _plain %t/only-plain64.a -o %t/no-fallback64.dylib 2>&1 \
# RUN:   | FileCheck %s --check-prefix=NO-ARCHIVE-FALLBACK
# NO-ARCHIVE-FALLBACK: ignoring file because it is universal (arm64) but does not contain the arm64e architecture

## Reverse fallback is equally invalid: ordinary ARM64 output must never
## extract ARM64e static archive members, including from a universal file.
# RUN: llvm-lipo -create %t/libauth.a -o %t/only-auth32.a
# RUN: llvm-lipo -create -fat64 %t/libauth.a -o %t/only-auth64.a
# RUN: not %no-arg-lld -arch arm64 -platform_version macos 13.0 13.0 \
# RUN:   -dylib -u _from_archive %t/only-auth32.a -o %t/no-reverse32.dylib 2>&1 \
# RUN:   | FileCheck %s --check-prefix=NO-REVERSE-ARCHIVE
# RUN: not %no-arg-lld -arch arm64 -platform_version macos 13.0 13.0 \
# RUN:   -dylib -u _from_archive %t/only-auth64.a -o %t/no-reverse64.dylib 2>&1 \
# RUN:   | FileCheck %s --check-prefix=NO-REVERSE-ARCHIVE
# NO-REVERSE-ARCHIVE: ignoring file because it is universal (arm64e) but does not contain the arm64 architecture

## A forged FAT64 descriptor cannot change the real ABI of an inner archive.
## The table's ARM64e claim may select a slice, but extracting its plain
## ARM64 object must still fail before an output is written.
# RUN: %python -c "import pathlib,struct,sys; b=bytearray(pathlib.Path(sys.argv[1]).read_bytes()); assert struct.unpack_from('>I',b,0)[0]==0xcafebabf; struct.pack_into('>I',b,12,0x80000002); pathlib.Path(sys.argv[2]).write_bytes(b)" %t/only-plain64.a %t/spoofed-arm64e.a
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -u _plain %t/spoofed-arm64e.a -o %t/spoofed.dylib 2>&1 \
# RUN:   | FileCheck %s --check-prefix=SPOOFED-ARCHIVE
# SPOOFED-ARCHIVE: unsupported arm64e pointer-authentication ABI

# ARCHS: arm64 arm64e
# ARM64: CpuSubType: CPU_SUBTYPE_ARM64_ALL (0x0)

#--- auth.s
.text
.globl _from_archive
_from_archive:
  ret

.data
.p2align 3
.globl _archive_signed_pointer
_archive_signed_pointer:
  .quad _from_archive@AUTH(ia,42,addr)

#--- client.s
.text
.globl _client
_client:
  bl _from_archive
  ret

.data
.p2align 3
.quad _from_archive@AUTH(db,7)

#--- plain.s
.text
.globl _plain
_plain:
  ret
