// RUN: %clang_cc1 -triple arm64e-apple-macosx13.0 -std=c++17 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple arm64-apple-macosx13.0 -std=c++17 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-apple-macosx13.0 -std=c++17 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple arm64e-apple-macosx13.0 -x objective-c++ -std=c++17 -emit-llvm -o - %s | FileCheck %s

struct Small { long x, y; };
struct Large { long x[8]; };
struct WithPointer { int *p; long n; };

// The ABI may pass these records in registers or indirectly. No pointer
// captures attribute should be inferred for their lowered arguments.
// CHECK-LABEL: define{{.*}} @take_small(
// CHECK-NOT: captures(
// CHECK: ret void
extern "C" void take_small(Small __attribute__((noescape)) s) {}

// CHECK-LABEL: define{{.*}} @take_large(
// CHECK-NOT: captures(
// CHECK: ret void
extern "C" void take_large(Large __attribute__((noescape)) s) {}

// CHECK-LABEL: define{{.*}} @take_record(
// CHECK-NOT: captures(
// CHECK: ret void
extern "C" void take_record(WithPointer __attribute__((noescape)) s) {}

// The existing pointer contract is retained.
// CHECK-LABEL: define{{.*}} @take_pointer(
// CHECK-SAME: ptr {{.*}}captures(address)
// CHECK: ret void
extern "C" void take_pointer(int * __attribute__((noescape)) p) {}
