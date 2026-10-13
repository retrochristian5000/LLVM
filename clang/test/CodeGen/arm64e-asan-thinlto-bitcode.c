// REQUIRES: aarch64-registered-target
//
// Check Clang's ARM64e ASan ThinLTO IR directly: sanitizer globals and shadow
// memory must coexist with signed function pointers and the versioned PAC ABI.
// ARM64 remains a negative control and must not gain ARM64e metadata.
//
// RUN: %clang -target arm64e-apple-macosx13.0 -O1 -flto=thin -fsanitize=address -emit-llvm -S %s -o - | FileCheck %s --check-prefix=ARM64E
// RUN: %clang -target arm64-apple-macosx13.0 -O1 -flto=thin -fsanitize=address -emit-llvm -S %s -o - | FileCheck %s --check-prefix=ARM64
//
// ARM64E: target triple = "arm64e-apple-macosx13.0.0"
// ARM64E: @target.ptrauth = private constant
// ARM64E: @callback = global
// ARM64E: @__asan_shadow_memory_dynamic_address = external global i64
// ARM64E: @__asan_global_callback = internal global
// ARM64E: "ptrauth"(i32 0, i64 0)
// ARM64E: !"ptrauth.abi-version"
//
// ARM64: target triple = "arm64-apple-macosx13.0.0"
// ARM64: @callback = global
// ARM64-NOT: @target.ptrauth
// ARM64: @__asan_shadow_memory_dynamic_address = external global i64
// ARM64: @__asan_global_callback = internal global
// ARM64-NOT: !"ptrauth.abi-version"

typedef int (*callback_t)(int);
extern int target(int);
callback_t callback = target;

__attribute__((noinline)) int probe(char *p, int x) {
  return p[x] + callback(x);
}
