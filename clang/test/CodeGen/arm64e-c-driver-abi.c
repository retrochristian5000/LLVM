// RUN: %clang -target arm64e-apple-macos -S -emit-llvm \
 // RUN:   -Xclang -disable-llvm-passes %s -o - | FileCheck %s

// Exercise the driver defaults and C CodeGen together.  The standard arm64e
// function-pointer ABI uses IA with discriminator zero.

extern int target(int);

// CHECK: @global_fp = global ptr ptrauth (ptr @target, i32 0)
int (*global_fp)(int) = target;

// CHECK-LABEL: define{{.*}} i32 @call_fp(
// CHECK: call i32 %{{.*}}(i32 noundef %{{.*}}) [ "ptrauth"(i32 0, i64 0) ]
int call_fp(int (*fp)(int), int value) {
  return fp(value);
}
