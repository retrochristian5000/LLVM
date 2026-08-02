; RUN: llc -mtriple=sparc64-linux-gnu -mattr=+reserve-l0 -verify-machineinstrs -o - %s | FileCheck %s --check-prefixes=CHECK-RESERVED-L0
; RUN: llc -mtriple=sparc64-linux-gnu -mattr=+reserve-l0 -verify-machineinstrs -stop-after=finalize-isel -o - %s | FileCheck %s --check-prefix=NAMES
; RUN: llc -mtriple=sparc64-linux-gnu -mattr=+reserve-l0 -verify-machineinstrs -stop-after=prolog-epilog -simplify-mir -o - %s | FileCheck %s --check-prefix=LIVEINS

;; Ensure explicit register references are catched as well.

; CHECK-RESERVED-L0: %l0
define void @set_reg(i32 zeroext %x) {
entry:
  tail call void @llvm.write_register.i32(metadata !0, i32 %x)
  ret void
}

declare void @llvm.write_register.i32(metadata, i32)
!0 = !{!"l0"}

; NAMES-LABEL: name: {{ *}}read_percent_sp
; NAMES: {{%[0-9]+}}:i64regs = COPY $o6
define i64 @read_percent_sp() {
entry:
  %value = call i64 @llvm.read_register.i64(metadata !1)
  ret i64 %value
}

; NAMES-LABEL: name: {{ *}}read_sp
; NAMES: {{%[0-9]+}}:i64regs = COPY $o6
define i64 @read_sp() {
entry:
  %value = call i64 @llvm.read_register.i64(metadata !2)
  ret i64 %value
}

; NAMES-LABEL: name: {{ *}}read_percent_r14
; NAMES: {{%[0-9]+}}:i64regs = COPY $o6
define i64 @read_percent_r14() {
entry:
  %value = call i64 @llvm.read_register.i64(metadata !3)
  ret i64 %value
}

; NAMES-LABEL: name: {{ *}}read_percent_fp
; NAMES: {{%[0-9]+}}:i64regs = COPY $o6
define i64 @read_percent_fp() {
entry:
  %value = call i64 @llvm.read_register.i64(metadata !4)
  ret i64 %value
}

; NAMES-LABEL: name: {{ *}}read_fp
; NAMES: {{%[0-9]+}}:i64regs = COPY $o6
define i64 @read_fp() {
entry:
  %value = call i64 @llvm.read_register.i64(metadata !5)
  ret i64 %value
}

; NAMES-LABEL: name: {{ *}}read_r30
; NAMES: {{%[0-9]+}}:i64regs = COPY $o6
define i64 @read_r30() {
entry:
  %value = call i64 @llvm.read_register.i64(metadata !6)
  ret i64 %value
}

; NAMES-LABEL: name: {{ *}}read_percent_g6
; NAMES: {{%[0-9]+}}:i64regs = COPY $g6
define i64 @read_percent_g6() {
entry:
  %value = call i64 @llvm.read_register.i64(metadata !7)
  ret i64 %value
}

; NAMES-LABEL: name: {{ *}}read_r0
; NAMES: {{%[0-9]+}}:i64regs = COPY $g0
define i64 @read_r0() {
entry:
  %value = call i64 @llvm.read_register.i64(metadata !8)
  ret i64 %value
}

; NAMES-LABEL: name: {{ *}}read_r16
; NAMES: {{%[0-9]+}}:i64regs = COPY $l0
; CHECK-RESERVED-L0-LABEL: read_r16:
; CHECK-RESERVED-L0: save %sp
; CHECK-RESERVED-L0: restore {{.*}}%l0
define i64 @read_r16() {
entry:
  %value = call i64 @llvm.read_register.i64(metadata !9)
  ret i64 %value
}

; LIVEINS-LABEL: name: {{ *}}read_r31
; LIVEINS-NOT: liveins:
; LIVEINS: bb.0.entry:
; LIVEINS-NOT: liveins:
; LIVEINS: $o0 = COPY $o7
define i64 @read_r31() {
entry:
  %value = call i64 @llvm.read_register.i64(metadata !10)
  ret i64 %value
}

; LIVEINS-LABEL: name: {{ *}}read_i7_nonentry
; LIVEINS: liveins:
; LIVEINS-NEXT: - { reg: '$o0' }
; LIVEINS-NEXT: - { reg: '$o1' }
; LIVEINS-NOT: reg: '$i{{[01]}}'
; LIVEINS: bb.0.entry:
; LIVEINS: liveins: $o0, $o1{{$}}
; LIVEINS: bb.{{[0-9]+}}.read:
; LIVEINS-NOT: liveins:
; LIVEINS: $o0 = COPY $o7
define i64 @read_i7_nonentry(i64 %zero_value, i1 %cond) {
entry:
  br i1 %cond, label %read, label %zero

read:
  %value = call i64 @llvm.read_register.i64(metadata !11)
  ret i64 %value

zero:
  ret i64 %zero_value
}

; NAMES-LABEL: name: {{ *}}read_i7_nonleaf
; NAMES: {{%[0-9]+}}:i64regs = COPY $i7
; CHECK-RESERVED-L0-LABEL: read_i7_nonleaf:
; CHECK-RESERVED-L0: save %sp
; CHECK-RESERVED-L0: call callee
; CHECK-RESERVED-L0: ret
define i64 @read_i7_nonleaf() {
entry:
  call void @callee()
  %value = call i64 @llvm.read_register.i64(metadata !11)
  ret i64 %value
}

declare i64 @llvm.read_register.i64(metadata)
declare void @callee()

!1 = !{!"%sp"}
!2 = !{!"sp"}
!3 = !{!"%r14"}
!4 = !{!"%fp"}
!5 = !{!"fp"}
!6 = !{!"r30"}
!7 = !{!"%g6"}
!8 = !{!"r0"}
!9 = !{!"r16"}
!10 = !{!"r31"}
!11 = !{!"i7"}
