; REQUIRES: aarch64
; RUN: rm -rf %t; split-file %s %t
; RUN: opt -module-summary %t/auth.ll -o %t/auth.o
; RUN: opt -module-summary %t/impl.ll -o %t/impl.o
; RUN: opt -module-summary %t/arm64.ll -o %t/arm64.o
;
;; Two ARM64e ThinLTO partitions must retain the signed callback initializer,
;; PAC-aware indirect call, versioned object subtype, and chained fixups.
; RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib -fixup_chains -object_path_lto %t/lto-objects \
; RUN:   %t/auth.o %t/impl.o -o %t/libauth.dylib
; RUN: llvm-readobj --file-headers %t/lto-objects/1.arm64e.lto.o | FileCheck %s --check-prefix=EOBJ
; RUN: llvm-objdump --macho --chained-fixups %t/libauth.dylib | FileCheck %s --check-prefix=EAUTH
; EOBJ: CpuSubType: CPU_SUBTYPE_ARM64E (0x2)
; EOBJ-NEXT: CpuSubTypeCapabilities: 0x80000000
; EOBJ-NEXT: PointerAuthABI: Userland
; EOBJ-NEXT: PointerAuthABIVersion: 0
; EAUTH: pointer_format = 12 (DYLD_CHAINED_PTR_ARM64E_USERLAND24)
;
;; The public LTO entrypoint must not treat ordinary ARM64 IR as ARM64e,
;; even though LLVM's Triple identifies both with the AArch64 instruction set.
; RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
; RUN:   -dylib %t/arm64.o -o %t/invalid-e.dylib 2>&1 | \
; RUN:   FileCheck %s --check-prefix=BAD-ARM64
; BAD-ARM64: arm64e output requires arm64e bitcode
; RUN: not %no-arg-lld -arch arm64 -platform_version macos 13.0 13.0 \
; RUN:   -dylib %t/auth.o -o %t/invalid-a.dylib 2>&1 | \
; RUN:   FileCheck %s --check-prefix=BAD-ARM64E
; BAD-ARM64E: arm64e bitcode is incompatible with an arm64 output
;
;; Separate ordinary ARM64 ThinLTO remains valid.
; RUN: %no-arg-lld -arch arm64 -platform_version macos 13.0 13.0 \
; RUN:   -dylib %t/arm64.o -o %t/libarm64.dylib
; RUN: llvm-readobj --file-headers %t/libarm64.dylib | FileCheck %s --check-prefix=AOBJ
; AOBJ: CpuSubType: CPU_SUBTYPE_ARM64_ALL (0x0)
;
;--- auth.ll
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64e-apple-macosx13.0.0"

@_whp_target.ptrauth = private constant { ptr, i32, i64, i64 } { ptr @_whp_target, i32 0, i64 0, i64 0 }, section "llvm.ptrauth", align 8
@_whp_callback = global ptr @_whp_target.ptrauth, align 8

declare i32 @_whp_target(i32)

define i32 @_whp_indirect(i32 %x) #0 {
entry:
  %cb = load ptr, ptr @_whp_callback, align 8
  %value = call i32 %cb(i32 %x) [ "ptrauth"(i32 0, i64 0) ]
  ret i32 %value
}

attributes #0 = { "ptrauth-calls" "ptrauth-returns" "ptrauth-auth-traps" }

!llvm.module.flags = !{!0}
!0 = !{i32 6, !"ptrauth.abi-version", !1}
!1 = !{!2}
!2 = !{i32 0, i1 false}

;--- impl.ll
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64e-apple-macosx13.0.0"

define i32 @_whp_target(i32 %x) #0 {
entry:
  %sum = add i32 %x, 7
  ret i32 %sum
}
attributes #0 = { "ptrauth-calls" "ptrauth-returns" }
!llvm.module.flags = !{!0}
!0 = !{i32 6, !"ptrauth.abi-version", !1}
!1 = !{!2}
!2 = !{i32 0, i1 false}

;--- arm64.ll
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx13.0.0"

define i32 @_whp_plain(i32 %x) {
entry:
  %result = add i32 %x, 7
  ret i32 %result
}
