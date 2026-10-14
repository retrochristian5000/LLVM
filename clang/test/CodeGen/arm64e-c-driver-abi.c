// RUN: %clang -target arm64e-apple-macos -S -emit-llvm \
// RUN:   -Xclang -disable-llvm-passes %s -o - | FileCheck %s
// RUN: %clang -target arm64-apple-macos -S -emit-llvm \
// RUN:   -Xclang -disable-llvm-passes %s -o - | FileCheck %s --check-prefix=ARM64

// Exercise the driver defaults and C CodeGen together.  The standard arm64e
// function-pointer ABI uses IA with discriminator zero.

extern int target(int);

// CHECK: @global_fp = global ptr ptrauth (ptr @target, i32 0)
// ARM64: @global_fp = global ptr @target
int (*global_fp)(int) = target;

// CHECK-LABEL: define{{.*}} i32 @call_fp(
// CHECK: call i32 %{{.*}}(i32 noundef %{{.*}}) [ "ptrauth"(i32 0, i64 0) ]
int call_fp(int (*fp)(int), int value) {
  return fp(value);
}

// ARM64-LABEL: define{{.*}} i32 @call_fp(
// ARM64: call i32 %{{.*}}(i32 noundef %{{.*}})
// ARM64-NOT: "ptrauth"
// ARM64: ret i32

// Direct calls must not acquire an indirect-call authentication bundle.
// CHECK-LABEL: define{{.*}} i32 @call_direct(
// CHECK: call i32 @target(i32 noundef %{{.*}})
// CHECK-NOT: "ptrauth"
// CHECK: ret i32
// ARM64-LABEL: define{{.*}} i32 @call_direct(
// ARM64: call i32 @target(i32 noundef %{{.*}})
int call_direct(int value) {
  return target(value);
}

// A function pointer loaded from global storage is also an indirect call.
// CHECK-LABEL: define{{.*}} i32 @call_global(
// CHECK: load ptr, ptr @global_fp
// CHECK: call i32 %{{.*}}(i32 noundef %{{.*}}) [ "ptrauth"(i32 0, i64 0) ]
// ARM64-LABEL: define{{.*}} i32 @call_global(
// ARM64: load ptr, ptr @global_fp
// ARM64: call i32 %{{.*}}(i32 noundef %{{.*}})
// ARM64-NOT: "ptrauth"
// ARM64: ret i32
int call_global(int value) {
  return global_fp(value);
}
