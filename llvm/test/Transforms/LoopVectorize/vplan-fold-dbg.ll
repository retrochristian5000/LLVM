; RUN: opt -p debugify,loop-vectorize -force-vector-width=4 -force-vector-interleave=1 -S \
; RUN:   -vplan-print-after=printOptimizedVPlan < %s 2> %t | FileCheck %s
; RUN: cat %t | FileCheck %s --check-prefix=VPLAN

; The debug attached to the urem instruction should remain after
; being folded to `and i64 %iv, 15`
define void @urem_fold(ptr %dst, ptr %src, i64 %n) {
; CHECK-LABEL: define void @urem_fold(
; CHECK:  [[VECTOR_BODY:.*:]]
; CHECK:    [[FOLD:%.*]] = and i64 {{.*}}, 15, !dbg [[DBG:![0-9]+]]
; CHECK:  [[LOOP:.*:]]
; CHECK:    [[ORIG:%.*]] = urem i64 [[IV:%.*]], 16, !dbg [[DBG:![0-9]+]]
;
; VPLAN-LABEL: VPlan for loop in 'urem_fold' after printOptimizedVPlan
; VPLAN: EMIT vp{{.*}} = and vp{{.*}}, ir<15>, !dbg
entry:
  br label %loop

loop:
  %iv = phi i64 [ 0, %entry ], [ %iv.next, %loop ]
  %urem = urem i64 %iv, 16
  %arrayidx = getelementptr inbounds nuw [4 x i8], ptr %src, i64 %urem
  %load1 = load i32, ptr %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds nuw [4 x i8], ptr %dst, i64 %iv
  %load2 = load i32, ptr %arrayidx2, align 4
  %add = add nsw i32 %load2, %load1
  store i32 %add, ptr %arrayidx2, align 4
  %iv.next = add nuw nsw i64 %iv, 1
  %exitcond.not = icmp eq i64 %iv.next, %n
  br i1 %exitcond.not, label %exit, label %loop

exit:
  ret void

}

