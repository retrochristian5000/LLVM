// On arm64e, an imported direct call must not request an inline GOT load
// that bypasses the authenticated linker stub.
// RUN: %clang_cc1 -triple arm64e-apple-ios -fptrauth-calls -fno-plt -emit-llvm %s -o - | FileCheck %s --check-prefix=AUTH
// RUN: %clang_cc1 -triple arm64-apple-ios -fno-plt -emit-llvm %s -o - | FileCheck %s --check-prefix=PLAIN

void external_function(void);

void caller(void) { external_function(); }

// AUTH: declare void @external_function()
// AUTH-NOT: nonlazybind
// PLAIN: declare void @external_function() [[NLB:#[0-9]+]]
// PLAIN: attributes [[NLB]] = { {{.*}}nonlazybind{{.*}} }
