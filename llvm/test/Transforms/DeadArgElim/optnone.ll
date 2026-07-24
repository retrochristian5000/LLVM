; RUN: opt -passes=deadargelim -S < %s | FileCheck %s

; CHECK-LABEL: define internal i32 @optnone_dead_arg(i32 %live, i32 %dead)
; CHECK-NOT: DW_CC_nocall
define internal i32 @optnone_dead_arg(i32 %live, i32 %dead) optnone noinline !dbg !4 {
  ret i32 %live
}

; CHECK-LABEL: define internal i32 @dead_arg(i32 %live)
define internal i32 @dead_arg(i32 %live, i32 %dead) {
  ret i32 %live
}

define i32 @caller() {
  %a = call i32 @optnone_dead_arg(i32 1, i32 2)
  %b = call i32 @dead_arg(i32 3, i32 4)
  %c = add i32 %a, %b
  ret i32 %c
}

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "optnone.c", directory: "/")
!2 = !DISubroutineType(types: !7)
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = distinct !DISubprogram(name: "optnone_dead_arg", scope: !1, file: !1, line: 1, type: !2, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0)
!7 = !{!8, !8, !8}
!8 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
