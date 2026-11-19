; REQUIRES: aarch64
; RUN: rm -rf %t; split-file %s %t
; RUN: llvm-as %t/lto.ll -o %t/lto.o
; RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macosx13.0 -o %t/native.o %t/native.s

;; The linker's independent -mcpu must not silently downgrade the arm64e
;; compiler's PAC feature floor when compiling bitcode.
; RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib %t/lto.o -o %t/default.dylib
; RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib -mcpu apple-a12 %t/lto.o -o %t/a12.dylib
; RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib -mcpu apple-a18 %t/lto.o -o %t/a18.dylib

; RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib -mcpu apple-a7 %t/lto.o -o %t/a7.dylib 2>&1 | FileCheck %s --check-prefix=BAD-A7
; BAD-A7: arm64e LTO CPU 'apple-a7' is incompatible with the arm64e ABI
; RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib -mcpu generic %t/lto.o -o %t/generic.dylib 2>&1 | FileCheck %s --check-prefix=BAD-GENERIC
; BAD-GENERIC: arm64e LTO CPU 'generic' is incompatible with the arm64e ABI

;; The advanced -mllvm path must not bypass the CPU floor or disable PAuth.
; RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib -mllvm -mcpu=apple-a7 %t/lto.o -o %t/mllvm-cpu.dylib 2>&1 | FileCheck %s --check-prefix=BAD-A7
; RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib -mcpu apple-a12 -mllvm -mattr=-pauth \
; RUN:   %t/lto.o -o %t/no-pauth.dylib 2>&1 | FileCheck %s --check-prefix=BAD-PAC
; BAD-PAC: is incompatible with the arm64e ABI: requires Armv8.3-A and pointer authentication
; RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib -mcpu apple-a12 -mllvm -mattr=-pauth,+pauth \
; RUN:   %t/lto.o -o %t/pauth-restored.dylib

;; A native-only ARM64e link must not be rejected by LTO-only CPU flags.
; RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib -mcpu apple-a7 %t/native.o -o %t/native.dylib

;; Ordinary ARM64 bitcode remains independent of arm64e's ABI floor.
; RUN: llvm-as %t/arm64.ll -o %t/arm64.o
; RUN: %no-arg-lld -arch arm64 -platform_version macos 13.0 13.0 \
; RUN:   -dylib -mcpu generic %t/arm64.o -o %t/arm64.dylib

;--- lto.ll
target triple = "arm64e-apple-macosx13.0.0"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"

define i32 @_whp_lto_target() {
  ret i32 42
}

;--- native.s
.text
.globl _whp_native_target
_whp_native_target:
  ret

;--- arm64.ll
target triple = "arm64-apple-macosx13.0.0"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"

define i32 @_whp_arm64_target() {
  ret i32 42
}
