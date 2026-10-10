# REQUIRES: aarch64

## Recognize X1 dylibs and stubs without inventing support for PAuth_LR
## code generation or silently treating an x1-only library as plain arm64e.

# RUN: rm -rf %t; split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/provider.o %t/provider.s
# RUN: llvm-mc -filetype=obj -triple=arm64-apple-macos -o %t/client.o %t/client.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 15 26 \
# RUN:   -dylib -install_name @rpath/libx1.dylib %t/provider.o -o %t/regular.dylib
## This is an intentionally synthetic header mutation for reader/guard tests,
## not a real PAuth_LR dylib: no executable x1 support is implied.
# RUN: %python -c "import struct,sys; b=bytearray(open(sys.argv[1],'rb').read()); struct.pack_into('<I',b,8,0x8000000c); open(sys.argv[2],'wb').write(b)" %t/regular.dylib %t/x1.dylib
# RUN: llvm-readobj -h %t/x1.dylib | FileCheck %s --check-prefix=HEADER
# RUN: llvm-objdump --macho --private-header %t/x1.dylib | FileCheck %s --check-prefix=OBJDUMP
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 15 26 \
# RUN:   -dylib %t/provider.o %t/x1.dylib -o %t/refuse.dylib 2>&1 | FileCheck %s --check-prefix=BIN-ERROR
# RUN: not %no-arg-lld -arch arm64e.x1 -platform_version macos 15 26 \
# RUN:   -dylib %t/provider.o -o %t/unsupported.dylib 2>&1 | FileCheck %s --check-prefix=OUTPUT-ERROR
# RUN: %no-arg-lld -arch arm64 -platform_version macos 15 26 \
# RUN:   -dylib %t/client.o %t/mixed.tbd -o %t/mixed.dylib
# RUN: not %no-arg-lld -arch arm64 -platform_version macos 15 26 \
# RUN:   -dylib %t/client.o %t/x1-only.tbd -o %t/unsupported-tbd.dylib 2>&1 | FileCheck %s --check-prefix=TBD-ERROR

# HEADER:      CpuSubType: CPU_SUBTYPE_ARM64E_X1 (0xC)
# HEADER-NEXT: CpuSubTypeCapabilities: 0x80000000
# HEADER-NEXT: PointerAuthABI: Userland
# HEADER-NEXT: PointerAuthABIVersion: 0
# OBJDUMP: E.X1
# BIN-ERROR: arm64e.x1 dylib requires an arm64e.x1 output
# OUTPUT-ERROR: -arch arm64e.x1 requires PAuth_LR code generation
# TBD-ERROR: is incompatible with arm64

#--- provider.s
.text
.globl _foo
_foo:
  ret

#--- client.s
.text
.globl _call_foo
_call_foo:
  bl _foo
  ret

#--- mixed.tbd
--- !tapi-tbd
tbd-version: 4
targets: [ arm64-macos, arm64e.x1-macos ]
install-name: '@rpath/libmixed.dylib'
exports:
  - targets: [ arm64-macos, arm64e.x1-macos ]
    symbols: [ _foo ]
...

#--- x1-only.tbd
--- !tapi-tbd
tbd-version: 4
targets: [ arm64e.x1-macos ]
install-name: '@rpath/libx1-only.dylib'
exports:
  - targets: [ arm64e.x1-macos ]
    symbols: [ _foo ]
...
