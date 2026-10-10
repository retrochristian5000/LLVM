# REQUIRES: aarch64

## Check binary dylibs separately from relocatable Mach-O inputs.
## An arm64e output can bind symbols from a versioned arm64e ABI 0 dylib
## or an ordinary arm64 dylib. Incompatible binary metadata must not
## enter its authenticated chained-fixup imports.

# RUN: rm -rf %t; split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/client.o %t/client.s
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/lib-arm64e.o %t/lib.s
# RUN: llvm-mc -filetype=obj -triple=arm64-apple-macos -o %t/lib-arm64.o %t/lib.s

# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -install_name @rpath/libfoo.dylib %t/lib-arm64e.o -o %t/libfoo.dylib
# RUN: %no-arg-lld -arch arm64 -platform_version macos 13.0 13.0 \
# RUN:   -dylib -install_name @rpath/libfoo-arm64.dylib %t/lib-arm64.o -o %t/libfoo-arm64.dylib

## Compatible arm64e and arm64 binaries must remain linkable.
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/client.o %t/libfoo.dylib -o %t/client-arm64e.dylib
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/client.o %t/libfoo-arm64.dylib -o %t/client-arm64.dylib
# RUN: llvm-objdump --macho --chained-fixups %t/client-arm64e.dylib | FileCheck %s --check-prefix=FIXUPS
# RUN: llvm-objdump --macho --chained-fixups %t/client-arm64.dylib | FileCheck %s --check-prefix=FIXUPS

# FIXUPS: pointer_format = 12 (DYLD_CHAINED_PTR_ARM64E_USERLAND24)
# FIXUPS: _foo

## Mutate only the Mach-O header's 32-bit cpu subtype field (byte 8).
## Verify that the binary dylib loader checks its actual ABI version.
# RUN: %python -c "import struct,sys; b=bytearray(open(sys.argv[1],'rb').read()); struct.pack_into('<I',b,8,int(sys.argv[3],16)); open(sys.argv[2],'wb').write(b)" %t/libfoo.dylib %t/lib-v1.dylib 0x81000002
# RUN: %python -c "import struct,sys; b=bytearray(open(sys.argv[1],'rb').read()); struct.pack_into('<I',b,8,int(sys.argv[3],16)); open(sys.argv[2],'wb').write(b)" %t/libfoo.dylib %t/lib-kernel.dylib 0xc0000002
# RUN: %python -c "import struct,sys; b=bytearray(open(sys.argv[1],'rb').read()); struct.pack_into('<I',b,8,int(sys.argv[3],16)); open(sys.argv[2],'wb').write(b)" %t/libfoo.dylib %t/lib-legacy.dylib 0x00000002

# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib %t/client.o %t/lib-v1.dylib -o %t/bad-v1.dylib 2>&1 | FileCheck %s --check-prefix=BAD-ABI
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib %t/client.o %t/lib-kernel.dylib -o %t/bad-kernel.dylib 2>&1 | FileCheck %s --check-prefix=BAD-ABI
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib %t/client.o %t/lib-legacy.dylib -o %t/bad-legacy.dylib 2>&1 | FileCheck %s --check-prefix=BAD-ABI

# BAD-ABI: unsupported arm64e dylib pointer-authentication ABI

## A different CPU family must not be mistaken for arm64e.  The synthetic
## wrong-CPU fixture needs no extra backend: alter CPU type and subtype.
# RUN: %python -c "import struct,sys; b=bytearray(open(sys.argv[1],'rb').read()); struct.pack_into('<II',b,4,0x01000007,3); open(sys.argv[2],'wb').write(b)" %t/libfoo.dylib %t/lib-wrong-cpu.dylib
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib %t/client.o %t/lib-wrong-cpu.dylib -o %t/bad-cpu.dylib 2>&1 | FileCheck %s --check-prefix=BAD-CPU
# BAD-CPU: dylib has architecture x86_64 which is incompatible with target architecture arm64e

#--- lib.s
.text
.globl _foo
_foo:
  ret

#--- client.s
.text
.globl _caller
_caller:
  bl _foo
  ret
