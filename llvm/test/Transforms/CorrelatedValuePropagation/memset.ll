; RUN: opt -passes=correlated-propagation -S < %s | FileCheck %s
; RUN: opt -passes='correlated-propagation,instcombine' -S < %s | FileCheck %s --check-prefix=COMBINED

declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg)

; A length in [0, 1] is guarded and specialized to one on the call path.
define void @range_0_1(ptr %dst, i8 %value, i64 %n) {
; CHECK-LABEL: define void @range_0_1(
; CHECK-SAME: ptr [[DST:%.*]], i8 [[VALUE:%.*]], i64 [[N:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[LEN:%.*]] = and i64 [[N]], 1
; CHECK-NEXT:    [[MEMSET_NOTZERO:%.*]] = icmp ne i64 [[LEN]], 0
; CHECK-NEXT:    br i1 [[MEMSET_NOTZERO]], label %[[DO_MEMSET:.*]], label %[[END:.*]]
; CHECK:       [[DO_MEMSET]]:
; CHECK-NEXT:    call void @llvm.memset.p0.i64(ptr align 1 [[DST]], i8 [[VALUE]], i64 1, i1 false)
; CHECK-NEXT:    br label %[[END]]
; CHECK:       [[END]]:
; CHECK-NEXT:    ret void
; COMBINED-LABEL: define void @range_0_1(
; COMBINED-SAME: ptr [[DST:%.*]], i8 [[VALUE:%.*]], i64 [[N:%.*]]) {
; COMBINED:      [[LEN:%.*]] = and i64 [[N]], 1
; COMBINED:      [[ISZERO:%.*]] = icmp eq i64 [[LEN]], 0
; COMBINED:      br i1 [[ISZERO]], label %[[END_BB:.]], label %[[STORE_BB:.]]
; COMBINED:      [[STORE_BB]]:
; COMBINED-NEXT: store i8 [[VALUE]], ptr [[DST]], align 1
; COMBINED-NEXT: br label %[[END_BB]]
; COMBINED:      [[END_BB]]:
; COMBINED-NOT:  @llvm.memset
; COMBINED:      ret void
entry:
  %len = and i64 %n, 1
  call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 %value, i64 %len, i1 false)
  ret void
}

; Volatile memsets keep volatile memory semantics after scalarization.
define void @range_0_1_volatile(ptr %dst, i8 %value, i64 %n) {
; CHECK-LABEL: define void @range_0_1_volatile(
; CHECK-SAME: ptr [[DST:%.*]], i8 [[VALUE:%.*]], i64 [[N:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[LEN:%.*]] = and i64 [[N]], 1
; CHECK-NEXT:    [[MEMSET_NOTZERO:%.*]] = icmp ne i64 [[LEN]], 0
; CHECK-NEXT:    br i1 [[MEMSET_NOTZERO]], label %[[DO_MEMSET:.*]], label %[[END:.*]]
; CHECK:       [[DO_MEMSET]]:
; CHECK-NEXT:    call void @llvm.memset.p0.i64(ptr align 1 [[DST]], i8 [[VALUE]], i64 1, i1 true)
; CHECK-NEXT:    br label %[[END]]
; CHECK:       [[END]]:
; CHECK-NEXT:    ret void
; COMBINED-LABEL: define void @range_0_1_volatile(
; COMBINED-SAME: ptr [[DST:%.*]], i8 [[VALUE:%.*]], i64 [[N:%.*]]) {
; COMBINED:      [[LEN:%.*]] = and i64 [[N]], 1
; COMBINED:      [[ISZERO:%.*]] = icmp eq i64 [[LEN]], 0
; COMBINED:      br i1 [[ISZERO]], label %[[END_BB:.]], label %[[STORE_BB:.]]
; COMBINED:      [[STORE_BB]]:
; COMBINED-NEXT: store volatile i8 [[VALUE]], ptr [[DST]], align 1
; COMBINED-NEXT: br label %[[END_BB]]
; COMBINED:      [[END_BB]]:
; COMBINED-NOT:  @llvm.memset
; COMBINED:      ret void
entry:
  %len = and i64 %n, 1
  call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 %value, i64 %len, i1 true)
  ret void
}

; The interval [0, 2] contains three values and must not be guarded.
define void @range_0_2(ptr %dst, i8 %value, i64 %n) {
; CHECK-LABEL: define void @range_0_2(
; CHECK-SAME: ptr [[DST:%.*]], i8 [[VALUE:%.*]], i64 [[N:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[LEN:%.*]] = urem i64 [[N]], 3
; CHECK-NEXT:    call void @llvm.memset.p0.i64(ptr align 1 [[DST]], i8 [[VALUE]], i64 [[LEN]], i1 false)
; CHECK-NEXT:    ret void
entry:
  %len = urem i64 %n, 3
  call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 %value, i64 %len, i1 false)
  ret void
}
