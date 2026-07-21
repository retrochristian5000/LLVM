; RUN: llc -O0 < %s -mtriple=nvptx64-nvidia-cuda | FileCheck %s
;;
;; An instruction's DILayerLoc references an intermediate DIFile that carries a
;; checksum (required by the verifier) but NO source: text. Source now lives on
;; DIFile.source, so a source-less intermediate file produces no .code_block.
;; When every intermediate file lacks source, the NVPTX backend produces no
;; section text, so the PTX output omits the .nv_intermediate_source_section
;; entirely instead of emitting an empty stub.

;; A non-empty PTX is still produced...
; CHECK: .target sm_{{[0-9]+}}
;; ... and the secondary intermediate location IS emitted (the file is
;; referenced) ...
; CHECK: .loc_intermediate
;; ... but because the intermediate file has no source:, the section header must
;; NOT appear anywhere in the output.
; CHECK-NOT: .nv_intermediate_source_section

define dso_local void @no_intermediate_source(ptr noundef %v) #0 !dbg !8 {
entry:
  %v.addr = alloca ptr, align 8
  store ptr %v, ptr %v.addr, align 8, !dbg !11
  ret void, !dbg !12
}

attributes #0 = { noinline optnone "target-cpu"="sm_75" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "test.cu", directory: "/test")
!2 = !{i32 7, !"Dwarf Version", i32 2}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!8 = distinct !DISubprogram(name: "no_intermediate_source", scope: !1, file: !1, line: 1, type: !9, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0)
!9 = !DISubroutineType(types: !10)
!10 = !{null}

;; Intermediate file: checksum present (required) but NO source: text, so no
;; code_block is built for it.
!4 = !DIFile(filename: "tileIR_source.unused", directory: ".", checksumkind: CSK_MD5, checksum: "cccccccccccccccccccccccccccccccc")
!5 = !DILayerLoc(line: 100, column: 10, file: !4, kind: "TileIR")
!6 = !DILayerLocList(!5)

;; The store references the source-less intermediate layer; the ret is source-only.
!11 = !DILocation(line: 2, column: 5, scope: !8, irlayers: !6)
!12 = !DILocation(line: 4, column: 1, scope: !8)
