// REQUIRES: aarch64-registered-target
//
// -fno-plt must not bypass authenticated linker stubs on arm64e, even
// when another flag disables optional pointer-authentication instrumentation.
// Ordinary arm64 retains its nonlazybind and runtime GOT optimizations.
//
// RUN: %clang_cc1 -triple arm64e-apple-ios -fno-plt -emit-llvm %s -o - | FileCheck %s --check-prefix=ARM64E --implicit-check-not=nonlazybind --implicit-check-not=RtLibUseGOT
// RUN: %clang_cc1 -triple arm64e-apple-ios -fno-plt -fno-ptrauth-calls -emit-llvm %s -o - | FileCheck %s --check-prefix=ARM64E --implicit-check-not=nonlazybind --implicit-check-not=RtLibUseGOT
// RUN: %clang_cc1 -triple arm64-apple-ios -fno-plt -emit-llvm %s -o - | FileCheck %s --check-prefix=ARM64
//
// ARM64E: declare void @external_function()
//
// ARM64: declare void @external_function() #[[ATTRS:[0-9]+]]
// ARM64: attributes #[[ATTRS]] = { {{.*}}nonlazybind
// ARM64: !{i32 7, !"RtLibUseGOT", i32 1}

void external_function(void);

void caller(void) {
  external_function();
}
