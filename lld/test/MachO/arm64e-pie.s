# REQUIRES: aarch64

## Exercise a real MH_EXECUTE image rather than only arm64e dylibs.
## PIE relocations need VM-offset rebases, authenticated pointer metadata,
## dynamic imports, the MH_PIE header flag, and a code signature on macOS.

# RUN: rm -rf %t; split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/main.o %t/main.s
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/lib.o %t/lib.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -install_name @rpath/libpie-test.dylib %t/lib.o -o %t/libpie-test.dylib
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -execute -pie -e _main %t/main.o %t/libpie-test.dylib -o %t/pie
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -execute -e _main %t/main.o %t/libpie-test.dylib -o %t/default-pie

## Offset 8: versioned ARM64e user ABI 0; offset 12: MH_EXECUTE;
## offset 24: Mach-O header flags, including MH_PIE (0x200000).
# RUN: %python -c "import struct,sys; h=open(sys.argv[1],'rb').read(32); assert struct.unpack_from('<I',h,8)[0] == 0x80000002; assert struct.unpack_from('<I',h,12)[0] == 2; assert struct.unpack_from('<I',h,24)[0] & 0x200000" %t/pie
# RUN: %python -c "import struct,sys; h=open(sys.argv[1],'rb').read(32); assert struct.unpack_from('<I',h,8)[0] == 0x80000002; assert struct.unpack_from('<I',h,12)[0] == 2; assert struct.unpack_from('<I',h,24)[0] & 0x200000" %t/default-pie

# RUN: llvm-objdump --macho --chained-fixups %t/pie | FileCheck %s --check-prefix=FIXUPS
# RUN: llvm-otool -l %t/pie | FileCheck %s --check-prefix=LOADS

# FIXUPS: chained fixups header (LC_DYLD_CHAINED_FIXUPS)
# FIXUPS: pointer_format = 12 (DYLD_CHAINED_PTR_ARM64E_USERLAND24)
# FIXUPS: _imported

# LOADS: cmd LC_DYLD_CHAINED_FIXUPS
# LOADS: cmd LC_MAIN
# LOADS: cmd LC_CODE_SIGNATURE

#--- lib.s
.text
.globl _imported
_imported:
  ret

#--- main.s
.text
.globl _main
_main:
  bl _imported
  mov w0, #0
  ret
_local:
  ret

.data
.p2align 3
.quad _main@AUTH(ia,42,addr)
.quad _local@AUTH(da,7,addr)
.quad _imported@AUTH(ia,77,addr)
.quad _main
