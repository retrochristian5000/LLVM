# REQUIRES: aarch64

## ARM64e libunwind authenticates C++ EH personality functions with the
## address of their GOT slot. The linker must supply dyld-authenticated
## slots for imported and local personality symbols, for dylibs and PIE.

# RUN: rm -rf %t; split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/provider.o %t/provider.s
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos \
# RUN:   -emit-compact-unwind-non-canonical=true -o %t/consumer.o %t/consumer.s
# RUN: llvm-objdump --macho --reloc %t/consumer.o | FileCheck %s --check-prefix=INPUT
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -install_name @rpath/libeh-provider.dylib %t/provider.o -o %t/libeh-provider.dylib
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib %t/consumer.o %t/libeh-provider.dylib -o %t/libconsumer.dylib
# RUN: llvm-objdump --macho --indirect-symbols %t/libconsumer.dylib | FileCheck %s --check-prefix=GOT
# RUN: llvm-objdump --macho --chained-fixups %t/libconsumer.dylib | FileCheck %s --check-prefix=FIXUP
# RUN: llvm-objdump --macho --dwarf=frames %t/libconsumer.dylib | FileCheck %s --check-prefix=DWARF
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -execute -e _main %t/consumer.o %t/libeh-provider.dylib -o %t/consumer
# RUN: llvm-objdump --macho --indirect-symbols %t/consumer | FileCheck %s --check-prefix=GOT
# RUN: llvm-objdump --macho --chained-fixups %t/consumer | FileCheck %s --check-prefix=FIXUP

# INPUT: Relocation information (__TEXT,__eh_frame)
# INPUT: PTRTGOT
# GOT: Indirect symbols for (__DATA,__auth_got)
# GOT: _imported_personality
# FIXUP: pointer_format = 12 (DYLD_CHAINED_PTR_ARM64E_USERLAND24)
# FIXUP: _imported_personality
# DWARF: Augmentation: "zPLR"
# DWARF: Personality Address:

#--- provider.s
.text
.globl _imported_personality
_imported_personality:
  ret

#--- consumer.s
.text
.globl _main, _local_personality
.p2align 2
_main:
  .cfi_startproc
  .cfi_personality 155, _imported_personality
  .cfi_lsda 16, Lexception0
  .cfi_def_cfa_offset 16
  ret
  .cfi_endproc

.p2align 2
_other:
  .cfi_startproc
  .cfi_personality 155, _local_personality
  .cfi_lsda 16, Lexception1
  .cfi_def_cfa_offset 16
  .cfi_escape 0
  ret
  .cfi_endproc

_local_personality:
  ret

.section __TEXT,__gcc_except_tab
Lexception0:
  .byte 0
Lexception1:
  .byte 0
