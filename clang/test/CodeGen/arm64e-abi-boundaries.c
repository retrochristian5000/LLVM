// REQUIRES: aarch64-registered-target
//
// PAuth-capable CPUs do not imply the arm64e pointer-authentication ABI.
// Keep the ordinary arm64 and arm64e driver, IR, and Mach-O output distinct.
// No SDK or runtime is required: these are compile-only checks.
//
// RUN: %clang --target=arm64-apple-macosx13.0 -O0 -S -emit-llvm -o - %s | \
// RUN:   FileCheck %s --check-prefix=ARM64-IR
// RUN: %clang --target=arm64e-apple-macosx13.0 -O0 -S -emit-llvm -o - %s | \
// RUN:   FileCheck %s --check-prefix=ARM64E-IR
// RUN: %clang --target=arm64-apple-macosx13.0 -c -o %t.arm64.o %s
// RUN: %clang --target=arm64e-apple-macosx13.0 -c -o %t.arm64e.o %s
// RUN: llvm-readobj --file-headers %t.arm64.o | \
// RUN:   FileCheck %s --check-prefix=ARM64-OBJ
// RUN: llvm-readobj --file-headers %t.arm64e.o | \
// RUN:   FileCheck %s --check-prefix=ARM64E-OBJ
//
// ARM64-IR-LABEL: define{{.*}} @call_pointer(
// ARM64-IR: call i32
// ARM64-IR-NOT: [ "ptrauth"
// ARM64-IR-LABEL: define{{.*}} @abi_probe(
// ARM64-IR: ret i32 0
// ARM64-IR-NOT: !"ptrauth.abi-version"
//
// ARM64E-IR-LABEL: define{{.*}} @call_pointer(
// ARM64E-IR: call i32 {{.*}}[ "ptrauth"(i32 0, i64 0) ]
// ARM64E-IR-LABEL: define{{.*}} @abi_probe(
// ARM64E-IR: ret i32 1
// ARM64E-IR: !"ptrauth.abi-version"
//
// ARM64-OBJ: CpuSubType: CPU_SUBTYPE_ARM64_ALL (0x0)
// ARM64E-OBJ: CpuSubType: CPU_SUBTYPE_ARM64E (0x2)
// ARM64E-OBJ-NEXT: CpuSubTypeCapabilities: 0x80000000
// ARM64E-OBJ-NEXT: PointerAuthABI: Userland
// ARM64E-OBJ-NEXT: PointerAuthABIVersion: 0

#if defined(__arm64e__)
#define WHP_ARM64E_ABI 1
#else
#define WHP_ARM64E_ABI 0
#endif

typedef int (*function_pointer)(int);
int call_pointer(function_pointer callback, int value) {
  return callback(value);
}

int abi_probe(void) { return WHP_ARM64E_ABI; }
