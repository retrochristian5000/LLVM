; REQUIRES: aarch64
; RUN: rm -rf %t; split-file %s %t
;
;; Two different IR names can denote one Mach-O linker symbol. A definition
;; @_Z6symboli and a verbatim reference @"\01__Z6symboli" both map to
;; __Z6symboli. The prevailing definition must remain resolvable regardless
;; of the order in which LTO sees the modules. This matters when C++ or
;; bindgen-generated declarations bypass ordinary IR name mangling.
;
; RUN: opt -module-summary %t/definition.ll -o %t/definition.thin.o
; RUN: opt -module-summary %t/reference.ll -o %t/reference.thin.o
; RUN: opt %t/definition.ll -o %t/definition.full.o
; RUN: opt %t/reference.ll -o %t/reference.full.o
;
;; In particular, the reference-first ThinLTO link used to internalize the
;; hidden prevailing definition before the final native link could see it.
; RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib -fixup_chains -u _main %t/reference.thin.o \
; RUN:   %t/definition.thin.o -o %t/ref-first.thin.dylib
; RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib -fixup_chains -u _main %t/definition.thin.o \
; RUN:   %t/reference.thin.o -o %t/def-first.thin.dylib
; RUN: llvm-objdump --syms %t/ref-first.thin.dylib | FileCheck %s --check-prefix=SYMBOLS
; RUN: llvm-objdump --syms %t/def-first.thin.dylib | FileCheck %s --check-prefix=SYMBOLS
; RUN: llvm-readobj --file-headers %t/ref-first.thin.dylib | FileCheck %s --check-prefix=ABI
; RUN: llvm-readobj --file-headers %t/def-first.thin.dylib | FileCheck %s --check-prefix=ABI
;
;; Regular LTO must not invent a different name or accidentally drop the
;; C++ definition when merging these two modules into one object.
; RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib -fixup_chains -u _main %t/reference.full.o \
; RUN:   %t/definition.full.o -o %t/ref-first.full.dylib
; RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib -fixup_chains -u _main %t/definition.full.o \
; RUN:   %t/reference.full.o -o %t/def-first.full.dylib
; RUN: llvm-objdump --syms %t/ref-first.full.dylib | FileCheck %s --check-prefix=SYMBOLS
; RUN: llvm-objdump --syms %t/def-first.full.dylib | FileCheck %s --check-prefix=SYMBOLS
;
;; The archive's index and lazy extraction must preserve the *same* linker
;; name. Linking the archive before or after its user must not change which
;; ARM64e IR definition prevails.
; RUN: llvm-ar rcs %t/definitions.a %t/definition.thin.o
; RUN: llvm-ranlib %t/definitions.a
; RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib -fixup_chains -u _main %t/reference.thin.o \
; RUN:   %t/definitions.a -o %t/ref-archive.dylib
; RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib -fixup_chains -u _main %t/definitions.a \
; RUN:   %t/reference.thin.o -o %t/archive-first.dylib
; RUN: llvm-objdump --syms %t/ref-archive.dylib | FileCheck %s --check-prefix=SYMBOLS
; RUN: llvm-objdump --syms %t/archive-first.dylib | FileCheck %s --check-prefix=SYMBOLS
;
;; A bare symbol-name match also accepts an undefined entry. These checks
;; require the prevailing definition and its caller in __TEXT,__text.
; SYMBOLS-DAG: {{[[:xdigit:]]+}} g{{[[:space:]]+}}F __TEXT,__text{{.*}} __Z6symboli
; SYMBOLS-DAG: {{[[:xdigit:]]+}} g{{[[:space:]]+}}F __TEXT,__text{{.*}} _main
; ABI: CpuSubType: CPU_SUBTYPE_ARM64E (0x2)
; ABI-NEXT: CpuSubTypeCapabilities: 0x80000000
; ABI-NEXT: PointerAuthABI: Userland
; ABI-NEXT: PointerAuthABIVersion: 0
;
;--- definition.ll
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64e-apple-macosx13.0.0"

define hidden i32 @_Z6symboli(i32 %value) #0 {
entry:
  ret i32 %value
}
attributes #0 = { noinline "ptrauth-calls" "ptrauth-returns" }
!llvm.module.flags = !{!0}
!0 = !{i32 6, !"ptrauth.abi-version", !1}
!1 = !{!2}
!2 = !{i32 0, i1 false}

;--- reference.ll
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64e-apple-macosx13.0.0"

declare i32 @"\01__Z6symboli"(i32)

define i32 @main() #0 {
entry:
  %result = call i32 @"\01__Z6symboli"(i32 42)
  ret i32 %result
}
attributes #0 = { "ptrauth-calls" "ptrauth-returns" }
!llvm.module.flags = !{!0}
!0 = !{i32 6, !"ptrauth.abi-version", !1}
!1 = !{!2}
!2 = !{i32 0, i1 false}
