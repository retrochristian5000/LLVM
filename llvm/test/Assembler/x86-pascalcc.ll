; RUN: llvm-as < %s | llvm-dis | FileCheck %s

; Keep the Pascal calling convention stable in textual IR until target-specific
; lowering is wired up.

declare cc 128 i32 @pascal_decl(i32, i32)

define cc 128 i32 @pascal_def(i32 %a, i32 %b) {
  %sum = add i32 %a, %b
  ret i32 %sum
}

; CHECK: declare cc 128 i32 @pascal_decl(i32, i32)
; CHECK: define cc 128 i32 @pascal_def(i32 %a, i32 %b) {
