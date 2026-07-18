; RUN: llc -mtriple=mipsel-unknown-linux-gnu -mcpu=mips32r5 -mattr=+fp64,+msa < %s | FileCheck %s --check-prefix=MIPS32
; RUN: llc -mtriple=mips64el-unknown-linux-gnuabi64 -mcpu=mips64r5 -mattr=+fp64,+msa -target-abi n64 < %s | FileCheck %s --check-prefix=MIPS64

; Test that f16 vectors can be passed as arguments and returned without crashing.
; This is a regression test for a crash in soft-promotion of BUILD_VECTOR operands.

define <8 x half> @f16vec_add(<8 x half> %a, <8 x half> %b) {
; MIPS32-LABEL: f16vec_add:
; MIPS32:         insert.w
; MIPS32:         insert.w
;
; MIPS64-LABEL: f16vec_add:
; MIPS64:         insert.d
; MIPS64:         insert.d
  %c = fadd <8 x half> %a, %b
  ret <8 x half> %c
}

define <4 x half> @f16vec4_add(<4 x half> %a, <4 x half> %b) {
; MIPS32-LABEL: f16vec4_add:
; MIPS32:         fill.h
; MIPS32:         fexupr.w
;
; MIPS64-LABEL: f16vec4_add:
; MIPS64:         fill.h
; MIPS64:         fexupr.w
  %c = fadd <4 x half> %a, %b
  ret <4 x half> %c
}

define <2 x half> @f16vec2_add(<2 x half> %a, <2 x half> %b) {
; MIPS32-LABEL: f16vec2_add:
; MIPS32:         fill.h
; MIPS32:         fexupr.w
;
; MIPS64-LABEL: f16vec2_add:
; MIPS64:         fill.h
; MIPS64:         fexupr.w
  %c = fadd <2 x half> %a, %b
  ret <2 x half> %c
}
