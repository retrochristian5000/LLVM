# RUN: split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=powerpc-none-elf -o %t/rom.o %t/rom.s
# RUN: ld.lld -m elf32ppc -z noexecstack --warn-common \
# RUN:   -T %t/rom.ld %t/rom.o -o %t/rom.elf
# RUN: llvm-readelf -hW %t/rom.elf | FileCheck %s --check-prefix=HDR
# RUN: llvm-readelf -lW %t/rom.elf | FileCheck %s --check-prefix=LOAD

# Verify LLD preserves the 32-bit PowerPC OpenBIOS firmware address space.
# The 0xfff00100 firmware entry and 0xfffffffc hard-reset word are distinct,
# and the final reset LOAD must not wrap beyond the ELF32 address space.
# Section/segment separation is required to preserve the PROM's W^X layout.

# HDR: Class: ELF32
# HDR: Data: 2's complement, big endian
# HDR: Machine: PowerPC
# HDR: Entry point address: 0xfff00100

# LOAD: LOAD{{.*}}0xfff00000{{.*}}R E
# LOAD-NEXT: LOAD{{.*}}0xfff09000{{.*}}R{{[[:space:]]+}}0x
# LOAD-NEXT: LOAD{{.*}}0xfff0a000{{.*}}RW
# LOAD-NEXT: LOAD{{.*}}0xfffffffc{{.*}}R E

#--- rom.s
.section .text.vectors,"ax"
.org 0x100
.globl _start
_start:
  b entry

.text
.globl entry
entry:
  lis 3, data_word@ha
  lwz 3, data_word@l(3)
  blr

.section .rodata,"a"
.asciz "openbios"

.data
.globl data_word
data_word:
  .long 42

.section .romentry,"ax"
  b entry

#--- rom.ld
OUTPUT_FORMAT(elf32-powerpc)
OUTPUT_ARCH(powerpc:common)
ENTRY(_start)

PHDRS {
  text PT_LOAD FLAGS(5);
  rodata PT_LOAD FLAGS(4);
  data PT_LOAD FLAGS(6);
  reset PT_LOAD FLAGS(5);
}

SECTIONS {
  . = 0xfff00000;
  .text.vectors : { *(.text.vectors) } :text
  . = 0xfff08000;
  .text : { *(.text) } :text
  .rodata ALIGN(4096) : { *(.rodata) } :rodata
  .data ALIGN(4096) : { *(.data) } :data
  _end = .;
  ASSERT(_end <= 0xfffffffc, "payload overlaps hard-reset vector")
  . = 0xfffffffc;
  .romentry : { *(.romentry) } :reset
  /DISCARD/ : { *(.comment*) *(.note.*) }
}
