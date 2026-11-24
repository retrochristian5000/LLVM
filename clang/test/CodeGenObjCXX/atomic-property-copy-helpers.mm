// RUN: %clang_cc1 -triple arm64-apple-macosx13.0 -std=c++11 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple arm64e-apple-macosx13.0 -std=c++11 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-apple-macosx13.0 -std=c++11 -emit-llvm -o - %s | FileCheck %s

struct Payload {
  int value;
  Payload();
  Payload(const Payload &);
  Payload &operator=(const Payload &);
};

__attribute__((objc_root_class))
@interface Owner
@property Payload first;
@property Payload second;
@property(nonatomic) Payload nonatomic;
@end

@implementation Owner
@synthesize first, second, nonatomic;
@end

// The two atomic properties share helpers of the same type, but the
// getter's copy constructor and setter's assignment must remain distinct.
// CHECK-LABEL: define internal void @__copy_helper_atomic_property_(
// CHECK: call {{.*}}@_ZN7PayloadC1ERKS_(
// CHECK: ret void
// CHECK: call void @objc_copyCppObjectAtomic({{.*}}@__copy_helper_atomic_property_
// CHECK-LABEL: define internal void @__assign_helper_atomic_property_(
// CHECK: call {{.*}}@_ZN7PayloadaSERKS_(
// CHECK: ret void
// CHECK: call void @objc_copyCppObjectAtomic({{.*}}@__assign_helper_atomic_property_
// CHECK: call void @objc_copyCppObjectAtomic({{.*}}@__copy_helper_atomic_property_
// CHECK: call void @objc_copyCppObjectAtomic({{.*}}@__assign_helper_atomic_property_
// CHECK-NOT: call void @objc_copyCppObjectAtomic(
