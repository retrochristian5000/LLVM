; Prevent ThinLTO from treating classic Windows ARM64 and ARM64EC as the
; same ABI. Each target generates AArch64 instructions, but they differ in
; variadic argument registers, reserved registers, and cross-ABI thunks.
;
; REQUIRES: aarch64
; RUN: split-file %s %t.dir
; RUN: opt -thinlto-bc %t.dir/native.ll -o %t.dir/native.bc
; RUN: opt -thinlto-bc %t.dir/ec.ll -o %t.dir/ec.bc
; RUN: not llvm-lto2 run %t.dir/native.bc %t.dir/ec.bc -o %t.dir/bad \
; RUN:   -r=%t.dir/native.bc,native_entry,px -r=%t.dir/ec.bc,ec_entry,px 2>&1 | \
; RUN:   FileCheck %s --check-prefix=BAD
; RUN: not llvm-lto2 run %t.dir/ec.bc %t.dir/native.bc -o %t.dir/bad-reverse \
; RUN:   -r=%t.dir/ec.bc,ec_entry,px -r=%t.dir/native.bc,native_entry,px 2>&1 | \
; RUN:   FileCheck %s --check-prefix=BAD
;
; BAD: cannot mix native Windows ARM64 and ARM64EC bitcode in one LTO link
;
;--- native.ll
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-p:64:64-i32:32-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "aarch64-unknown-windows-msvc"

define i32 @native_entry() {
entry:
  ret i32 1
}
;
;--- ec.ll
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-p:64:64-i32:32-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64ec-unknown-windows-msvc"

define i32 @ec_entry() {
entry:
  ret i32 2
}
