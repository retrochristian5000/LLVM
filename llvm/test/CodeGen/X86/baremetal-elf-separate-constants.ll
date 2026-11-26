; RUN: llc -mtriple=i386-none-elf -data-sections -x86-elf-separate-constants < %s | FileCheck %s --check-prefix=SPLIT
; RUN: llc -mtriple=i386-none-code16 -data-sections -x86-elf-separate-constants < %s | FileCheck %s --check-prefix=SPLIT
; RUN: llc -mtriple=i386-none-elf -data-sections < %s | FileCheck %s --check-prefix=MERGE
; RUN: llc -mtriple=i386-none-elf -x86-elf-separate-constants < %s | FileCheck %s --check-prefix=MERGE
; RUN: llc -mtriple=i386-unknown-linux-gnu -data-sections -x86-elf-separate-constants < %s | FileCheck %s --check-prefix=MERGE
; RUN: llc -mtriple=x86_64-none-elf -data-sections -x86-elf-separate-constants < %s | FileCheck %s --check-prefix=MERGE
;
; Keep constants independently collectible without changing hosted ELF,
; explicit sections, or the default section policy.
@literal_a = private unnamed_addr constant [6 x i8] c"alpha\00", align 1
@literal_b = private unnamed_addr constant [5 x i8] c"beta\00", align 1
@named = private unnamed_addr constant [7 x i8] c"pinned\00", section ".rodata.pinned", align 1

define ptr @get_a() {
  ret ptr @literal_a
}
define ptr @get_b() {
  ret ptr @literal_b
}
define ptr @get_named() {
  ret ptr @named
}

; SPLIT-DAG: .section .rodata.{{[^,]*}}literal_a,"a",@progbits
; SPLIT-DAG: .section .rodata.{{[^,]*}}literal_b,"a",@progbits
; SPLIT-DAG: .section .rodata.pinned,"a",@progbits
; MERGE-DAG: .section .rodata.str1.1,"aMS",@progbits,1
; MERGE-DAG: .section .rodata.pinned,"a",@progbits
