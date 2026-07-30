; RUN: llc -O0 < %s -mtriple=nvptx64-nvidia-cuda -mcpu=sm_70 -mattr=+ptx72 \
; RUN:   | FileCheck %s

;; Regression test for the DwarfDebug::beginInstruction dedup gate.
;;
;; DebugLoc::isSameSourceLocation used to compare only line/column/scope/
;; inlinedAt. The intermediate coordinate lives on the
;; DILocation's `irlayers` operand, so two instructions can share the same
;; source coordinate (line 10, col 5, same scope) yet carry DIFFERENT layers.
;; If isSameSourceLocation ignored irlayers, the two consecutive MachineInstrs
;; would be deduped and the second .loc_intermediate silently dropped from the
;; emitted PTX. isSameSourceLocation now also compares getRawIRLayers().
;;
;; This test pins both .loc_intermediate directives into the output so the
;; dedup gate is forced to consider irlayers differences in its equivalence
;; check.

target triple = "nvptx64-nvidia-cuda"

define i32 @dedup_gate_demo(i32 %a, i32 %b) !dbg !5 {
  %1 = add i32 %a, %b, !dbg !20
  %2 = mul i32 %1, 3,  !dbg !21
  ret i32 %2,          !dbg !100
}

;; Both instructions share the primary source coordinate (line 10, col 5), but
;; their `irlayers` differ (layer @ line 100 vs @ line 200). Because
;; isSameSourceLocation now compares irlayers, the dedup gate does NOT fire:
;; the primary .loc is re-emitted for the second instruction and each distinct
;; .loc_intermediate is emitted in source order.

; CHECK-LABEL: dedup_gate_demo
;; First instruction (add): source + intermediate @ line 100.
; CHECK:      .loc 1 10 5
; CHECK-NEXT: .loc_intermediate {{[0-9]+}} 100 1
;; Second instruction (mul): primary must be re-emitted (proves the dedup gate
;; did NOT fire, because the irlayers operand differs) and the intermediate @
;; line 200 must follow.
; CHECK:      .loc 1 10 5
; CHECK-NEXT: .loc_intermediate {{[0-9]+}} 200 1

!llvm.dbg.cu                    = !{!2}
!llvm.module.flags              = !{!0, !1}

!0 = !{i32 2, !"Dwarf Version", i32 2}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3,
                              emissionKind: DebugDirectivesOnly)
!3 = !DIFile(filename: "demo.c", directory: "/tmp")
!4 = !DISubroutineType(types: !{})
!5 = distinct !DISubprogram(name: "dedup_gate_demo", scope: !3, file: !3,
                            line: 1, type: !4, scopeLine: 1,
                            spFlags: DISPFlagDefinition, unit: !2)

!10 = !DIFile(filename: "demo.tile.ir", directory: "/tmp", checksumkind: CSK_MD5, checksum: "dddddddddddddddddddddddddddddddd")

;; Two distinct intermediate layers on the shared source coordinate.
!110 = !DILayerLoc(line: 100, column: 1, file: !10, kind: "tile ir")
!111 = !DILayerLoc(line: 200, column: 1, file: !10, kind: "tile ir")
!120 = !DILayerLocList(!110)
!121 = !DILayerLocList(!111)

;; Same source (line 10, col 5, scope !5) but different layer lists.
!20 = !DILocation(line: 10, column: 5, scope: !5, irlayers: !120)
!21 = !DILocation(line: 10, column: 5, scope: !5, irlayers: !121)

;; Bare source location (no layers).
!100 = !DILocation(line: 10, column: 5, scope: !5)
