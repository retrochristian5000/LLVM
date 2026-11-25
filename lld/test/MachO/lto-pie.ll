; REQUIRES: x86
;
;; LTO must retain a live pointer initializer across module boundaries and
;; leave executable PIE policy to the final Mach-O link. Exercise both full
;; LTO and ThinLTO, including relinks against the same native-object cache.
;
; RUN: rm -rf %t; split-file %s %t
; RUN: llvm-as %t/main.ll -o %t/main.full.o
; RUN: llvm-as %t/target.ll -o %t/target.full.o
; RUN: %lld -pie -no_fixup_chains %t/main.full.o %t/target.full.o -o %t/full-pie
; RUN: llvm-readobj --file-headers %t/full-pie | FileCheck %s --check-prefix=PIE
; RUN: obj2yaml %t/full-pie | FileCheck %s --check-prefix=REBASE
; RUN: %lld -no_pie %t/main.full.o %t/target.full.o -o %t/full-fixed
; RUN: llvm-readobj --file-headers %t/full-fixed | FileCheck %s --check-prefix=EXEC --implicit-check-not=MH_PIE
;
; RUN: opt -module-hash -module-summary %t/main.ll -o %t/main.thin.o
; RUN: opt -module-hash -module-summary %t/target.ll -o %t/target.thin.o
; RUN: mkdir %t/cache
; RUN: %lld -pie -no_fixup_chains -cache_path_lto %t/cache %t/main.thin.o %t/target.thin.o -o %t/thin-pie
; RUN: llvm-readobj --file-headers %t/thin-pie | FileCheck %s --check-prefix=PIE
; RUN: obj2yaml %t/thin-pie | FileCheck %s --check-prefix=REBASE
; RUN: ls %t/cache/llvmcache-* | count 2
; RUN: %lld -no_pie -cache_path_lto %t/cache %t/main.thin.o %t/target.thin.o -o %t/thin-fixed
; RUN: llvm-readobj --file-headers %t/thin-fixed | FileCheck %s --check-prefix=EXEC --implicit-check-not=MH_PIE
; RUN: ls %t/cache/llvmcache-* | count 2
; RUN: %lld -pie -no_fixup_chains -cache_path_lto %t/cache %t/main.thin.o %t/target.thin.o -o %t/thin-pie-again
; RUN: llvm-readobj --file-headers %t/thin-pie-again | FileCheck %s --check-prefix=PIE
; RUN: obj2yaml %t/thin-pie-again | FileCheck %s --check-prefix=REBASE
; RUN: ls %t/cache/llvmcache-* | count 2
;
; EXEC: FileType: Executable
; PIE: FileType: Executable
; PIE: MH_PIE
; REBASE: RebaseOpcodes:
; REBASE: REBASE_OPCODE_DO_REBASE
;
;--- main.ll
target triple = "x86_64-apple-macosx11.0.0"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"

@callback = global ptr @target, align 8
declare i32 @target()

define i32 @main() {
  %fn = load volatile ptr, ptr @callback, align 8
  %result = call i32 %fn()
  ret i32 %result
}

!llvm.module.flags = !{!0}
!0 = !{i32 8, !"PIC Level", i32 2}

;--- target.ll
target triple = "x86_64-apple-macosx11.0.0"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"

define i32 @target() {
  ret i32 7
}

!llvm.module.flags = !{!0}
!0 = !{i32 8, !"PIC Level", i32 2}
