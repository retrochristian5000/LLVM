; Prevent ThinLTO from cross-importing and optimizing IR across Darwin arm64
; and arm64e pointer-authentication ABI boundaries, in either module order.
;
; REQUIRES: aarch64
; RUN: split-file %s %t.dir
; RUN: opt -thinlto-bc %t.dir/arm64.ll -o %t.dir/arm64.bc
; RUN: opt -thinlto-bc %t.dir/arm64e.ll -o %t.dir/arm64e.bc
; RUN: not llvm-lto2 run %t.dir/arm64.bc %t.dir/arm64e.bc -o %t.dir/bad \
; RUN:   -r=%t.dir/arm64.bc,plain_entry,px \
; RUN:   -r=%t.dir/arm64e.bc,auth_entry,px 2>&1 | FileCheck %s --check-prefix=BAD
; RUN: not llvm-lto2 run %t.dir/arm64e.bc %t.dir/arm64.bc -o %t.dir/reverse \
; RUN:   -r=%t.dir/arm64e.bc,auth_entry,px \
; RUN:   -r=%t.dir/arm64.bc,plain_entry,px 2>&1 | FileCheck %s --check-prefix=BAD
;
; BAD: cannot mix Darwin arm64 and arm64e bitcode in one LTO link
;
;--- arm64.ll
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx13.0.0"
define i32 @plain_entry() {
entry:
  ret i32 1
}
;
;--- arm64e.ll
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64e-apple-macosx13.0.0"
define i32 @auth_entry() #0 {
entry:
  ret i32 2
}
attributes #0 = { "ptrauth-calls" "ptrauth-returns" }
!llvm.module.flags = !{!0}
!0 = !{i32 6, !"ptrauth.abi-version", !1}
!1 = !{!2}
!2 = !{i32 0, i1 false}
