# REQUIRES: aarch64

## arm64e uses the ARM64 direct-branch safe ICF thunk implementation.
## Address-significant duplicate functions retain distinct entry points;
## the final dylib still carries the arm64e user ABI and authenticated fixups.

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/main.o %t/main.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains --icf=safe_thunks %t/main.o \
# RUN:   -o %t/libicf.dylib -map %t/libicf.map
# RUN: llvm-objdump --no-print-imm-hex --macho -d --no-show-raw-insn \
# RUN:   --section="__TEXT,__text" %t/libicf.dylib | \
# RUN:   FileCheck %s --check-prefix=THUNK
# RUN: cat %t/libicf.map | FileCheck %s --check-prefix=MAP
# RUN: llvm-objdump --macho --chained-fixups %t/libicf.dylib | \
# RUN:   FileCheck %s --check-prefix=FIXUPS
# RUN: %python -c "import struct,sys; h=open(sys.argv[1],'rb').read(32); assert struct.unpack_from('<I',h,8)[0] == 0x80000002" %t/libicf.dylib

# THUNK-LABEL: _first:
# THUNK:       nop
# THUNK-LABEL: _second:
# THUNK-NEXT:  b _first

# MAP: 0x00000004 [  2] _second
# FIXUPS: pointer_format = 12 (DYLD_CHAINED_PTR_ARM64E_USERLAND24)

#--- main.s
.subsections_via_symbols
.text
.globl _first
_first:
.rept 8
  nop
.endr
  ret

.globl _second
_second:
.rept 8
  nop
.endr
  ret

.data
.p2align 3
.quad _first@AUTH(ia,42,addr)
.quad _second@AUTH(ia,42,addr)
