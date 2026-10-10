; Preserve the ABI split between ordinary Windows ARM64 and ARM64EC.
; For a variadic function, ARM64EC uses only x0-x3 for named parameters,
; with x4 pointing at stack arguments. Classic ARM64 keeps the fifth
; argument in x4. Nonvariadic ARM64EC keeps the normal ARM64 convention.
;
; RUN: llc -mtriple=aarch64-pc-windows-msvc < %s | FileCheck %s --check-prefix=CLASSIC
; RUN: llc -mtriple=arm64ec-pc-windows-msvc -arm64ec-generate-thunks=false < %s | FileCheck %s --check-prefix=EC

define win64cc i64 @fifth_variadic(i64 %a, i64 %b, i64 %c, i64 %d, i64 %fifth, ...) nounwind {
; CLASSIC-LABEL: fifth_variadic:
; CLASSIC: mov x0, x4
; EC-LABEL: fifth_variadic:
; EC: ldr x0, [x4]
  ret i64 %fifth
}

define win64cc i64 @fifth_fixed(i64 %a, i64 %b, i64 %c, i64 %d, i64 %fifth) nounwind {
; CLASSIC-LABEL: fifth_fixed:
; CLASSIC: mov x0, x4
; EC-LABEL: fifth_fixed:
; EC: mov x0, x4
  ret i64 %fifth
}
