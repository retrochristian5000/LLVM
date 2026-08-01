; RUN: llc < %s -mtriple=sparc   | FileCheck --check-prefix=sparc32 %s
; RUN: llc < %s -mtriple=sparcv9 | FileCheck --check-prefix=sparc64 %s

declare ptr @llvm.stackaddress.p0()

define ptr @test() {
; sparc32-LABEL: test:
; sparc32:       retl
; sparc32-NEXT:  add %sp, 68, %o0
;
; sparc64-LABEL: test:
; sparc64:       retl
; sparc64-NEXT:  add %sp, 2175, %o0
  %sp = call ptr @llvm.stackaddress.p0()
  ret ptr %sp
}
