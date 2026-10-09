; RUN: llc -mtriple=aarch64-windows-gnu -filetype=asm -o - < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=aarch64-windows-gnu -filetype=obj -o %t.obj < %s
; RUN: llvm-readobj --relocations %t.obj | FileCheck %s --check-prefix=RELOC

; A dso_local extern_weak reference can resolve to an under-aligned COFF
; definition, so LOADgot must not encode a scaled PAGEOFFSET_12L against it.
; The ADD page offset is unscaled and leaves the final load at offset zero.

target triple = "aarch64-windows-gnu"

declare extern_weak dso_local ptr @weak_fn()

define dso_local ptr @weak_address() {
; ASM-LABEL: weak_address:
; ASM:       adrp [[REG:x[0-9]+]], weak_fn
; ASM-NEXT:  add [[REG]], [[REG]], :lo12:weak_fn
; ASM-NEXT:  ldr [[REG]], [{{.*}}]
; ASM-NEXT:  ret
  ret ptr @weak_fn
}

; RELOC: IMAGE_REL_ARM64_PAGEBASE_REL21 weak_fn
; RELOC: IMAGE_REL_ARM64_PAGEOFFSET_12A weak_fn
; RELOC-NOT: IMAGE_REL_ARM64_PAGEOFFSET_12L weak_fn
