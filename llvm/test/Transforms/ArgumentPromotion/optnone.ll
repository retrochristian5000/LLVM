; RUN: opt -passes=argpromotion -S < %s | FileCheck %s

declare void @sink(i32)

; CHECK-LABEL: define internal void @optnone_promote(ptr %X)
; CHECK-NOT: DW_CC_nocall
define internal void @optnone_promote(ptr %X) optnone noinline !dbg !4 {
  %v = load i32, ptr %X, align 4
  call void @sink(i32 %v)
  ret void
}

; CHECK-LABEL: define internal void @promote(i32 %X.0.val)
define internal void @promote(ptr %X) {
  %v = load i32, ptr %X, align 4
  call void @sink(i32 %v)
  ret void
}

define void @caller(ptr %Y, ptr %Z) {
  call void @optnone_promote(ptr %Y)
  call void @promote(ptr %Z)
  ret void
}

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "optnone.c", directory: "/")
!2 = !DISubroutineType(types: !5)
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = distinct !DISubprogram(name: "optnone_promote", scope: !1, file: !1, line: 1, type: !2, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0)
!5 = !{null, !6}
!6 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !7, size: 64)
!7 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
