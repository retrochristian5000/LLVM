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

## llvm-mc emits the versioned arm64e user ABI 0 subtype 0x80000002.
## Verify the raw Mach-O header too, since objdump prints only "ARM64 E".
# RUN: %python -c "import struct,sys; assert struct.unpack_from('<I',open(sys.argv[1],'rb').read(),8)[0] == 0x80000002" %t.dylib

## Incompatible versions / kernel ABI / legacy unversioned ARM64e inputs
## must fail, rather than being relabelled as versioned user ABI 0.
# RUN: %python -c "import struct,sys; b=bytearray(open(sys.argv[1],'rb').read()); struct.pack_into('<I',b,8,int(sys.argv[3],16)); open(sys.argv[2],'wb').write(b)" %t.o %t-v1.o 0x81000002
# RUN: %python -c "import struct,sys; b=bytearray(open(sys.argv[1],'rb').read()); struct.pack_into('<I',b,8,int(sys.argv[3],16)); open(sys.argv[2],'wb').write(b)" %t.o %t-kernel.o 0xc0000002
# RUN: %python -c "import struct,sys; b=bytearray(open(sys.argv[1],'rb').read()); struct.pack_into('<I',b,8,int(sys.argv[3],16)); open(sys.argv[2],'wb').write(b)" %t.o %t-legacy.o 0x00000002
# RUN: %python -c "import struct,sys; b=bytearray(open(sys.argv[1],'rb').read()); struct.pack_into('<I',b,8,int(sys.argv[3],16)); open(sys.argv[2],'wb').write(b)" %t.o %t-arm64.o 0x00000000
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib %t-v1.o -o %t-v1.dylib 2>&1 | FileCheck %s --check-prefix=UNSUPPORTED
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib %t-kernel.o -o %t-kernel.dylib 2>&1 | FileCheck %s --check-prefix=UNSUPPORTED
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib %t-legacy.o -o %t-legacy.dylib 2>&1 | FileCheck %s --check-prefix=UNSUPPORTED
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib %t-arm64.o -o %t-arm64.dylib 2>&1 | FileCheck %s --check-prefix=UNSUPPORTED
# RUN: not %no-arg-lld -arch arm64 -platform_version macos 13.0 13.0 -dylib %t.o -o %t-wrong-arch.dylib 2>&1 | FileCheck %s --check-prefix=WRONG-ARCH
# UNSUPPORTED: unsupported arm64e pointer-authentication ABI
# WRONG-ARCH: arm64e object is incompatible with an arm64 output
