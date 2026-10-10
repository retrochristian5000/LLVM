; Test the ObjC intrinsic lowering stage separately from Clang.
; RUN: opt -mtriple=arm64e-apple-ios -passes=pre-isel-intrinsic-lowering -S -o - %s | FileCheck %s --check-prefix=AUTH
; RUN: opt -mtriple=arm64-apple-ios -passes=pre-isel-intrinsic-lowering -S -o - %s | FileCheck %s --check-prefix=PLAIN

define ptr @retain(ptr %arg) {
entry:
  %result = call ptr @llvm.objc.retain(ptr %arg)
  ret ptr %result
}

define void @release(ptr %arg) {
entry:
  call void @llvm.objc.release(ptr %arg)
  ret void
}

declare ptr @llvm.objc.retain(ptr)
declare void @llvm.objc.release(ptr)

; AUTH-DAG: declare ptr @objc_retain(ptr)
; AUTH-DAG: declare void @objc_release(ptr)
; AUTH-NOT: nonlazybind

; PLAIN-DAG: declare ptr @objc_retain(ptr) [[NLB:#[0-9]+]]
; PLAIN-DAG: declare void @objc_release(ptr) [[NLB]]
; PLAIN: attributes [[NLB]] = { nonlazybind }
