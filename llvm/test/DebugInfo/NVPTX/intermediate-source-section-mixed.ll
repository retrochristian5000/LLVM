; RUN: llc -O0 < %s -mtriple=nvptx64-nvidia-cuda | FileCheck %s
;;
;; Mixed intermediate files: one carries DIFile.source, the other only a checksum
;; (no source). Both are referenced -- so both get a .loc_intermediate and a
;; .file entry -- but the source-less file is SKIPPED in the source section (no
;; .code_block for it) while the sourced file's .code_block is still emitted.
;; This isolates the per-file skip (source-less -> no .code_block, section still
;; emitted) from the whole-section omission covered by
;; intermediate-source-section-empty.ll (where EVERY file is source-less).

;; Both intermediate files are referenced, so both get a .loc_intermediate...
; CHECK-DAG: .loc_intermediate [[FA:[0-9]+]] 100 10
; CHECK-DAG: .loc_intermediate [[FB:[0-9]+]] 200 20
;; ...and both get a .file entry: the sourced file by its checksum digest, the
;; source-less file by its filename (nothing to content-address without source).
; CHECK-DAG: .file [[FA]] ".{{/|\\\\}}aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
; CHECK-DAG: .file [[FB]] ".{{/|\\\\}}bbb.tileir"

;; The section is emitted with EXACTLY ONE code_block -- the sourced file's. The
;; source-less file is skipped: the section closes right after the single
;; code_block (a code_block for the source-less file would break this chain).
; CHECK: .nv_intermediate_source_section {
; CHECK-NEXT: .code_block {
; CHECK-NEXT: .ir_name: "tile ir"
; CHECK-NEXT: .sourceFileName: [[FA]]
; CHECK-NEXT: .source: <<< aaa source line >>>
; CHECK-NEXT: }
; CHECK-NEXT: }

define dso_local ptx_kernel void @test_kernel(ptr noundef %v) #0 !dbg !8 {
entry:
  %v.addr = alloca ptr, align 8
  store ptr %v, ptr %v.addr, align 8, !dbg !20
  store ptr null, ptr %v.addr, align 8, !dbg !21
  ret void, !dbg !22
}

attributes #0 = { noinline optnone "target-cpu"="sm_75" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "test.cu", directory: "/test")
!2 = !{i32 7, !"Dwarf Version", i32 2}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!8 = distinct !DISubprogram(name: "test_kernel", scope: !1, file: !1, line: 1, type: !9, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0)
!9 = !DISubroutineType(types: !10)
!10 = !{null}

;; Sourced intermediate file (referenced first -> lower .file number).
!14 = !DIFile(filename: "aaa.tileir", directory: ".", checksumkind: CSK_MD5, checksum: "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", source: "aaa source line")
!15 = !DILayerLoc(line: 100, column: 10, file: !14, kind: "tile ir")
!16 = !DILayerLocList(!15)

;; Source-less intermediate file (checksum only, no source:).
!24 = !DIFile(filename: "bbb.tileir", directory: ".", checksumkind: CSK_MD5, checksum: "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb")
!25 = !DILayerLoc(line: 200, column: 20, file: !24, kind: "tile ir")
!26 = !DILayerLocList(!25)

;; First instruction references the sourced file; second the source-less file.
!20 = !DILocation(line: 2, column: 5, scope: !8, irlayers: !16)
!21 = !DILocation(line: 3, column: 5, scope: !8, irlayers: !26)
!22 = !DILocation(line: 4, column: 1, scope: !8)
