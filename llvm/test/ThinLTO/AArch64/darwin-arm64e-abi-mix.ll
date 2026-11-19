; Prevent ThinLTO from cross-importing and optimizing IR across Darwin arm64
; and arm64e pointer-authentication ABI boundaries, in either module order.
;
; REQUIRES: aarch64
; RUN: split-file %s %t.dir
; RUN: opt -thinlto-bc %t.dir/arm64.ll -o %t.dir/arm64.bc
; RUN: opt -thinlto-bc %t.dir/arm64e.ll -o %t.dir/arm64e.bc
; RUN: opt -thinlto-bc %t.dir/arm64e-x1.ll -o %t.dir/arm64e-x1.bc
; RUN: not llvm-lto2 run %t.dir/arm64.bc %t.dir/arm64e.bc -o %t.dir/bad \
; RUN:   -r=%t.dir/arm64.bc,plain_entry,px \
; RUN:   -r=%t.dir/arm64e.bc,auth_entry,px 2>&1 | FileCheck %s --check-prefix=BAD
; RUN: not llvm-lto2 run %t.dir/arm64e.bc %t.dir/arm64.bc -o %t.dir/reverse \
; RUN:   -r=%t.dir/arm64e.bc,auth_entry,px \
; RUN:   -r=%t.dir/arm64.bc,plain_entry,px 2>&1 | FileCheck %s --check-prefix=BAD
;
; Legacy ThinLTO has a separate addModule() entry point; it must reject
; incompatible PAC ABIs before backend code generation, in both orders.
; RUN: not llvm-lto -thinlto-action=run %t.dir/arm64.bc %t.dir/arm64e.bc 2>&1 | FileCheck %s --check-prefix=LEGACY
; RUN: not llvm-lto -thinlto-action=run %t.dir/arm64e.bc %t.dir/arm64.bc 2>&1 | FileCheck %s --check-prefix=LEGACY
;
; ARM64e.x1 remains metadata-only: the optimizer must reject its ABI
; mismatch before attempting unsupported x1 code generation.
; RUN: not llvm-lto2 run %t.dir/arm64.bc %t.dir/arm64e-x1.bc -o %t.dir/bad-x1 \
; RUN:   -r=%t.dir/arm64.bc,plain_entry,px \
; RUN:   -r=%t.dir/arm64e-x1.bc,x1_entry,px 2>&1 | FileCheck %s --check-prefix=BAD
; RUN: not llvm-lto2 run %t.dir/arm64e-x1.bc %t.dir/arm64.bc -o %t.dir/reverse-x1 \
; RUN:   -r=%t.dir/arm64e-x1.bc,x1_entry,px \
; RUN:   -r=%t.dir/arm64.bc,plain_entry,px 2>&1 | FileCheck %s --check-prefix=BAD
; RUN: not llvm-lto -thinlto-action=run %t.dir/arm64.bc %t.dir/arm64e-x1.bc 2>&1 | FileCheck %s --check-prefix=LEGACY
; RUN: not llvm-lto -thinlto-action=run %t.dir/arm64e-x1.bc %t.dir/arm64.bc 2>&1 | FileCheck %s --check-prefix=LEGACY
;
; BAD: cannot mix Darwin arm64 and arm64e bitcode in one LTO link
; LEGACY: cannot mix Darwin arm64 and arm64e bitcode in one ThinLTO link
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

;
;--- arm64e-x1.ll
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64e.x1-apple-macosx13.0.0"
define i32 @x1_entry() #0 {
entry:
  ret i32 3
}
attributes #0 = { "ptrauth-calls" "ptrauth-returns" }
!llvm.module.flags = !{!0}
!0 = !{i32 6, !"ptrauth.abi-version", !1}
!1 = !{!2}
!2 = !{i32 0, i1 false}
