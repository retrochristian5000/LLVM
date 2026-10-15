; RUN: llc -mtriple=arm64ec-windows-msvc -filetype=obj -o %t.ec.obj %s
; RUN: llvm-readobj --sections %t.ec.obj | FileCheck %s --check-prefix=COFF
; RUN: llc -mtriple=aarch64-windows-msvc -filetype=obj -o %t.arm.obj %s
; RUN: llvm-readobj --sections %t.arm.obj | FileCheck %s --check-prefix=COFF
; RUN: llc -mtriple=x86_64-windows-msvc -filetype=obj -o %t.x64.obj %s
; RUN: llvm-readobj --sections %t.x64.obj | FileCheck %s --check-prefix=COFF
;
; Explicit BSS sections in ARM64EC and both hybrid peers must not occupy raw
; data bytes or acquire initialized-data COFF flags.

@uninitialized = global i32 0, section ".whp$bss", align 4 #0
@initialized = global i32 7, section ".whp$data", align 4
attributes #0 = { "bss-section"=".whp$bss" }

; COFF: Name: .whp$bss
; COFF: PointerToRawData: 0x0
; COFF: IMAGE_SCN_CNT_UNINITIALIZED_DATA
; COFF: Name: .whp$data
; COFF: IMAGE_SCN_CNT_INITIALIZED_DATA
