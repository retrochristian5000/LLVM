; RUN: not --crash opt -disable-output %s 2>&1 | FileCheck %s
;;
;; A DILayerLoc's file must carry a checksum (any kind): NVPTX emit uses the
;; checksum digest as the secondary .file name, so a checksum-less intermediate
;; DIFile is a HARD verifier error (a plain Check, not a soft CheckDI debug-info
;; error). The module is rejected rather than having its debug info stripped:
;; the hard error fires at parse time via UpgradeDebugInfo, which reports it
;; through report_fatal_error ("Broken module found") -- hence `not --crash`.
;; (llvm-as parses with debug-info upgrade disabled and does not surface it.)

; CHECK: intermediate DILayerLoc file requires a checksum

define void @k(ptr %p) !dbg !5 {
  store ptr null, ptr %p, align 8, !dbg !20
  ret void
}

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, emissionKind: FullDebug)
!1 = !DIFile(filename: "test.cu", directory: "/test")
!2 = !{i32 7, !"Dwarf Version", i32 2}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !DISubroutineType(types: !{null})
!5 = distinct !DISubprogram(name: "k", scope: !1, file: !1, line: 1, type: !4, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0)

;; Intermediate file with NO checksum -- the invariant under test.
!10 = !DIFile(filename: "kernel.tileir", directory: ".")
!11 = !DILayerLoc(line: 42, column: 5, file: !10, kind: "tile ir")
!12 = !DILayerLocList(!11)

!20 = !DILocation(line: 2, column: 5, scope: !5, irlayers: !12)
