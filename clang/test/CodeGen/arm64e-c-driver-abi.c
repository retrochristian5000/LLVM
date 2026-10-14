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

// The standard arm64e C function-pointer schema is storage-address independent.
// Raw byte copies (including overlapping memmove) must remain valid: introducing
// address-based signing or a PAC-specific memcpy ABI would break C interop.
typedef int (*Callback)(int);
struct Callbacks {
  Callback first;
  Callback second;
};

// CHECK-LABEL: define{{.*}} void @copy_callbacks(
// CHECK: call void @llvm.memcpy.p0.p0.i64({{.*}}i64 16, i1 false)
// CHECK-NOT: @llvm.ptrauth.resign
// CHECK: ret void
// ARM64-LABEL: define{{.*}} void @copy_callbacks(
// ARM64: call void @llvm.memcpy.p0.p0.i64({{.*}}i64 16, i1 false)
void copy_callbacks(struct Callbacks *dst, const struct Callbacks *src) {
  __builtin_memcpy(dst, src, sizeof(*dst));
}

// CHECK-LABEL: define{{.*}} void @move_callbacks(
// CHECK: call void @llvm.memmove.p0.p0.i64({{.*}}i64 16, i1 false)
// CHECK-NOT: @llvm.ptrauth.resign
// CHECK: ret void
// ARM64-LABEL: define{{.*}} void @move_callbacks(
// ARM64: call void @llvm.memmove.p0.p0.i64({{.*}}i64 16, i1 false)
void move_callbacks(struct Callbacks *dst, const struct Callbacks *src) {
  __builtin_memmove(dst, src, sizeof(*dst));
}

// Clearing callback storage must keep the ordinary null representation.
// CHECK-LABEL: define{{.*}} void @zero_callbacks(
// CHECK: call void @llvm.memset.p0.i64({{.*}}i8 0, i64 16, i1 false)
// CHECK: ret void
// ARM64-LABEL: define{{.*}} void @zero_callbacks(
// ARM64: call void @llvm.memset.p0.i64({{.*}}i8 0, i64 16, i1 false)
void zero_callbacks(struct Callbacks *dst) {
  __builtin_memset(dst, 0, sizeof(*dst));
}

// The eventual call must still authenticate a pointer loaded from memory.
// CHECK-LABEL: define{{.*}} i32 @call_stored_callback(
// CHECK: call i32 %{{.*}}(i32 noundef %{{.*}}) [ "ptrauth"(i32 0, i64 0) ]
// ARM64-LABEL: define{{.*}} i32 @call_stored_callback(
// ARM64: call i32 %{{.*}}(i32 noundef %{{.*}})
// ARM64-NOT: "ptrauth"
// ARM64: ret i32
int call_stored_callback(const struct Callbacks *src, int value) {
  return src->first(value);
}

#if defined(__arm64e__)
// Explicit address-discriminated fields have different copy semantics.
// Aggregate assignment must re-sign against the destination field address,
// including a null-preserving path, rather than using raw llvm.memcpy.
struct AddressSigned {
  int * __ptrauth(1,1,50) ptr;
};
struct UniformSigned {
  int * __ptrauth(1,0,50) ptr;
};

// CHECK-LABEL: define{{.*}} void @copy_address_signed(
// CHECK: call void @__copy_assignment_
// CHECK-LABEL: define{{.*}} void @__copy_assignment_
// CHECK: call i64 @llvm.ptrauth.blend(i64
// CHECK: call i64 @llvm.ptrauth.blend(i64
// CHECK: icmp ne ptr {{%.*}}, null
// CHECK: call i64 @llvm.ptrauth.resign(i64
// CHECK: phi ptr [ null,
void copy_address_signed(struct AddressSigned *dst,
                         const struct AddressSigned *src) {
  *dst = *src;
}

// Unlike address-diverse fields, explicitly non-address-discriminated
// pointers should still use a normal aggregate memory copy.
// CHECK-LABEL: define{{.*}} void @copy_uniform_signed(
// CHECK: call void @llvm.memcpy.p0.p0.i64({{.*}}i64 8, i1 false)
void copy_uniform_signed(struct UniformSigned *dst,
                         const struct UniformSigned *src) {
  *dst = *src;
}
#endif
