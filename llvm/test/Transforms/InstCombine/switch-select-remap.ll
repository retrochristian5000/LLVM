; RUN: opt < %s -passes=instcombine -S | FileCheck %s

; The compared value 4 has no explicit case, and the remapped value 6 maps to
; a real (non-default) case, so switching on %x needs a new explicit case for
; 4 pointing to bb2.
define void @test_remap_add_case(i8 %x) {
; CHECK-LABEL: define void @test_remap_add_case(
; CHECK-SAME: i8 [[X:%.*]]) {
; CHECK-NEXT:    switch i8 [[X]], label [[BB1:%.*]] [
; CHECK-NEXT:      i8 6, label [[BB2:%.*]]
; CHECK-NEXT:      i8 10, label [[BB3:%.*]]
; CHECK-NEXT:      i8 4, label [[BB2]]
; CHECK-NEXT:    ]
;
  %cmp = icmp eq i8 %x, 4
  %key = select i1 %cmp, i8 6, i8 %x
  switch i8 %key, label %bb1 [
    i8 6, label %bb2
    i8 10, label %bb3
  ]

bb1:
  call void @func1()
  unreachable
bb2:
  call void @func2()
  unreachable
bb3:
  call void @func3()
  unreachable
}

; The value 4 already has an explicit case pointing to bb4, but %key can never
; actually be 4 (it's remapped to 6 whenever %x is 4), so that case is really
; dead and should be retargeted to wherever the remapped value 6 dispatches to
; (bb2). bb4 loses its only predecessor, but the fold doesn't clean up its now-dead
; body itself (that's left to a follow-up DCE/SimplifyCFG run), so this
; function needs instcombine-no-verify-fixpoint to accept the one-pass result.
define void @test_remap_retarget_case(i8 %x) #0 {
; CHECK-LABEL: define void @test_remap_retarget_case(
; CHECK-SAME: i8 [[X:%.*]]) #[[ATTR0:[0-9]+]] {
; CHECK-NEXT:    switch i8 [[X]], label [[BB1:%.*]] [
; CHECK-NEXT:      i8 4, label [[BB2:%.*]]
; CHECK-NEXT:      i8 6, label [[BB2]]
; CHECK-NEXT:      i8 10, label [[BB3:%.*]]
; CHECK-NEXT:    ]
;
  %cmp = icmp eq i8 %x, 4
  %key = select i1 %cmp, i8 6, i8 %x
  switch i8 %key, label %bb1 [
    i8 4, label %bb4
    i8 6, label %bb2
    i8 10, label %bb3
  ]

bb1:
  call void @func1()
  unreachable
bb2:
  call void @func2()
  unreachable
bb3:
  call void @func3()
  unreachable
bb4:
  call void @func4()
  unreachable
}

; Same remap expressed with icmp ne / select(cond, %x, 6). The remapped value 6
; maps to a real (non-default) case, so switching on %x needs a new explicit
; case for 4 pointing to bb2, same as test_remap_add_case but via the NE arm.
define void @test_remap_ne_add_case(i8 %x) {
; CHECK-LABEL: define void @test_remap_ne_add_case(
; CHECK-SAME: i8 [[X:%.*]]) {
; CHECK-NEXT:    switch i8 [[X]], label [[BB1:%.*]] [
; CHECK-NEXT:      i8 6, label [[BB2:%.*]]
; CHECK-NEXT:      i8 10, label [[BB3:%.*]]
; CHECK-NEXT:      i8 4, label [[BB2]]
; CHECK-NEXT:    ]
;
  %cmp = icmp ne i8 %x, 4
  %key = select i1 %cmp, i8 %x, i8 6
  switch i8 %key, label %bb1 [
    i8 6, label %bb2
    i8 10, label %bb3
  ]

bb1:
  call void @func1()
  unreachable
bb2:
  call void @func2()
  unreachable
bb3:
  call void @func3()
  unreachable
}

; Negative test: the select's non-constant arm (%y) doesn't match the icmp's
; non-constant operand (%x), so this isn't a same-value remap and must not
; be folded.
define void @test_remap_mismatched_operand(i8 %x, i8 %y) {
; CHECK-LABEL: define void @test_remap_mismatched_operand(
; CHECK-SAME: i8 [[X:%.*]], i8 [[Y:%.*]]) {
; CHECK-NEXT:    [[CMP:%.*]] = icmp eq i8 [[X]], 4
; CHECK-NEXT:    [[KEY:%.*]] = select i1 [[CMP]], i8 6, i8 [[Y]]
; CHECK-NEXT:    switch i8 [[KEY]], label [[BB1:%.*]] [
; CHECK-NEXT:      i8 6, label [[BB2:%.*]]
; CHECK-NEXT:      i8 10, label [[BB3:%.*]]
; CHECK-NEXT:    ]
;
  %cmp = icmp eq i8 %x, 4
  %key = select i1 %cmp, i8 6, i8 %y
  switch i8 %key, label %bb1 [
    i8 6, label %bb2
    i8 10, label %bb3
  ]

bb1:
  call void @func1()
  unreachable
bb2:
  call void @func2()
  unreachable
bb3:
  call void @func3()
  unreachable
}

declare void @func1()
declare void @func2()
declare void @func3()
declare void @func4()

attributes #0 = { "instcombine-no-verify-fixpoint" }
