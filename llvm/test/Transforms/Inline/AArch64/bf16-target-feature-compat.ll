; RUN: opt -mtriple=aarch64-apple-macosx13.0.0 -passes=inline -S %s | FileCheck %s
;
; BF16-enabled callees must not be inlined into functions with an explicitly
; disabled BF16 feature, even when the callee requests alwaysinline. Conversely
; the same callee may be inlined into a BF16-capable caller.
; This protects ThinLTO-imported functions, which retain per-function features.

define float @bf16_callee(float %x) #0 {
entry:
  %sum = fadd float %x, 1.000000e+00
  ret float %sum
}

define float @compatible_caller(float %x) #1 {
entry:
  %v = call float @bf16_callee(float %x)
  ret float %v
}

; CHECK-LABEL: define float @compatible_caller(
; CHECK-NOT: call float @bf16_callee
; CHECK: fadd float

define float @incompatible_caller(float %x) #2 {
entry:
  %v = call float @bf16_callee(float %x)
  ret float %v
}

; CHECK-LABEL: define float @incompatible_caller(
; CHECK: call float @bf16_callee

attributes #0 = { alwaysinline "target-cpu"="apple-a18-pro" "target-features"="+bf16,+neon" }
attributes #1 = { "target-cpu"="apple-a18-pro" "target-features"="+bf16,+neon" }
attributes #2 = { "target-cpu"="apple-a18-pro" "target-features"="-bf16,+neon" }
