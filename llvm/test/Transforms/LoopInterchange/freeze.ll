; A freeze whose execution count changes under interchange can resample poison
; and break the correlation shared by its uses. Reject that nest, but retain
; interchange for an otherwise-identical profitable control.
;
; RUN: opt < %s -passes='loop(loop-interchange),print<loops>' \
; RUN:     -cache-line-size=64 -disable-output 2>&1 \
; RUN:     | FileCheck %s --check-prefix=LOOPS
; RUN: opt < %s -passes=loop-interchange -cache-line-size=64 \
; RUN:     -pass-remarks-output=%t.yaml -disable-output
; RUN: FileCheck %s --check-prefix=REMARK --input-file=%t.yaml
; RUN: llvm-extract -S -func=outer_header_freeze %s -o %t.freeze
; RUN: opt -S -passes=no-op-loopnest %t.freeze -o %t.noop
; RUN: opt -S -passes=loop-interchange -cache-line-size=64 \
; RUN:     %t.freeze -o %t.out
; RUN: diff -u %t.noop %t.out

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @profitable_control(ptr noalias %A, ptr noalias %R) {
entry:
  br label %outer.header

outer.header:
  %i = phi i64 [ 0, %entry ], [ %i.next, %outer.latch ]
  %sum.i = phi double [ 0.000000e+00, %entry ], [ %sum.next, %outer.latch ]
  br label %inner.header

inner.header:
  %j = phi i64 [ 0, %outer.header ], [ %j.next, %inner.header ]
  %sum.j = phi double [ %sum.i, %outer.header ], [ %sum.next.j, %inner.header ]
  %idx = getelementptr inbounds [4 x double], ptr %A, i64 %j, i64 %i
  %value = load double, ptr %idx, align 8
  %sum.next.j = fadd reassoc double %sum.j, %value
  %j.next = add i64 %j, 1
  %j.ec = icmp eq i64 %j.next, 4
  br i1 %j.ec, label %outer.latch, label %inner.header

outer.latch:
  %sum.next = phi double [ %sum.next.j, %inner.header ]
  %i.next = add i64 %i, 1
  %i.ec = icmp eq i64 %i.next, 4
  br i1 %i.ec, label %exit, label %outer.header

exit:
  %sum.result = phi double [ %sum.next, %outer.latch ]
  store double %sum.result, ptr %R, align 8
  ret void
}

; LOOPS-LABEL: Loop info for function 'profitable_control':
; LOOPS:         Loop at depth 1 containing: %inner.header<header>
; LOOPS-NEXT:      Loop at depth 2 containing: %outer.header<header>
; REMARK:      --- !Passed
; REMARK:      Name:            Interchanged
; REMARK-NEXT: Function:        profitable_control

define void @outer_preheader_freeze(ptr noalias %A, ptr noalias %R) {
entry:
  %choice = freeze i1 poison
  br label %outer.header

outer.header:
  %i = phi i64 [ 0, %entry ], [ %i.next, %outer.latch ]
  %sum.i = phi double [ 0.000000e+00, %entry ], [ %sum.next, %outer.latch ]
  br label %inner.header

inner.header:
  %j = phi i64 [ 0, %outer.header ], [ %j.next, %inner.header ]
  %sum.j = phi double [ %sum.i, %outer.header ], [ %sum.next.j, %inner.header ]
  %idx = getelementptr inbounds [4 x double], ptr %A, i64 %j, i64 %i
  %value = load double, ptr %idx, align 8
  %selected = select i1 %choice, double %value, double 0.000000e+00
  %sum.next.j = fadd reassoc double %sum.j, %selected
  %j.next = add i64 %j, 1
  %j.ec = icmp eq i64 %j.next, 4
  br i1 %j.ec, label %outer.latch, label %inner.header

outer.latch:
  %sum.next = phi double [ %sum.next.j, %inner.header ]
  %i.next = add i64 %i, 1
  %i.ec = icmp eq i64 %i.next, 4
  br i1 %i.ec, label %exit, label %outer.header

exit:
  %sum.result = phi double [ %sum.next, %outer.latch ]
  store double %sum.result, ptr %R, align 8
  ret void
}

; LOOPS-LABEL: Loop info for function 'outer_preheader_freeze':
; LOOPS:         Loop at depth 1 containing: %inner.header<header>
; LOOPS-NEXT:      Loop at depth 2 containing: %outer.header<header>
; REMARK:      --- !Passed
; REMARK:      Name:            Interchanged
; REMARK-NEXT: Function:        outer_preheader_freeze

define void @outer_header_freeze(ptr noalias %A, ptr noalias %R) {
entry:
  br label %outer.header

outer.header:
  %i = phi i64 [ 0, %entry ], [ %i.next, %outer.latch ]
  %sum.i = phi double [ 0.000000e+00, %entry ], [ %sum.next, %outer.latch ]
  %choice = freeze i1 poison
  br label %inner.header

inner.header:
  %j = phi i64 [ 0, %outer.header ], [ %j.next, %inner.header ]
  %sum.j = phi double [ %sum.i, %outer.header ], [ %sum.next.j, %inner.header ]
  %idx = getelementptr inbounds [4 x double], ptr %A, i64 %j, i64 %i
  %value = load double, ptr %idx, align 8
  %selected = select i1 %choice, double %value, double 0.000000e+00
  %sum.next.j = fadd reassoc double %sum.j, %selected
  %j.next = add i64 %j, 1
  %j.ec = icmp eq i64 %j.next, 4
  br i1 %j.ec, label %outer.latch, label %inner.header

outer.latch:
  %sum.next = phi double [ %sum.next.j, %inner.header ]
  %i.next = add i64 %i, 1
  %i.ec = icmp eq i64 %i.next, 4
  br i1 %i.ec, label %exit, label %outer.header

exit:
  %sum.result = phi double [ %sum.next, %outer.latch ]
  store double %sum.result, ptr %R, align 8
  ret void
}

; LOOPS-LABEL: Loop info for function 'outer_header_freeze':
; LOOPS:         Loop at depth 1 containing: %outer.header<header>
; LOOPS-NEXT:      Loop at depth 2 containing: %inner.header<header>
; REMARK:      --- !Missed
; REMARK:      Name:            UnsafeInst
; REMARK-NEXT: Function:        outer_header_freeze
