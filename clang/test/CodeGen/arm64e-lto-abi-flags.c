// REQUIRES: aarch64-registered-target
//
// Arm64e does not encode the effect of -fno-ptrauth-* in its target triple.
// Record pointer-representation ABI choices in bitcode with Error module-flag
// behavior. Native code is unchanged; incompatible Full LTO merges fail.

// RUN: %clang --target=arm64e-apple-macosx15.0 -flto=full -S -emit-llvm %s -o - | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang --target=arm64e-apple-macosx15.0 -flto=thin -S -emit-llvm %s -o - | FileCheck %s --check-prefix=DEFAULT
// DEFAULT: !{i32 1, !"arm64e-ptrauth-abi-v1", i32 799}

// RUN: %clang --target=arm64e-apple-macosx15.0 -flto=full -fno-ptrauth-calls -S -emit-llvm %s -o - 2>/dev/null | FileCheck %s --check-prefix=NO-CALLS
// NO-CALLS: !{i32 1, !"arm64e-ptrauth-abi-v1", i32 0}

// RUN: %clang --target=arm64e-apple-macosx15.0 -flto=thin -fno-ptrauth-vtable-pointer-type-discrimination -S -emit-llvm %s -o - 2>/dev/null | FileCheck %s --check-prefix=NO-VTABLE-TYPE
// NO-VTABLE-TYPE: !{i32 1, !"arm64e-ptrauth-abi-v1", i32 795}

// RUN: %clang --target=arm64-apple-macosx15.0 -flto=full -S -emit-llvm %s -o - | FileCheck %s --check-prefix=PLAIN --implicit-check-not='arm64e-ptrauth-abi-v1'
// PLAIN: target triple = "arm64-apple-macosx15.0.0"

// RUN: %clang --target=arm64e-unknown-darwin24.0.0 -flto=full -S -emit-llvm %s -o - | FileCheck %s --check-prefix=CLONE
// CLONE: !{i32 1, !"arm64e-ptrauth-abi-v1", i32 799}

// RUN: %clang --target=arm64e-apple-macosx15.0 -flto=full -emit-llvm -c %s -DABI_FUNCTION=arm64e_lto_default -o %t.default.bc
// RUN: %clang --target=arm64e-apple-macosx15.0 -flto=full -emit-llvm -c %s -DABI_FUNCTION=arm64e_lto_matching -o %t.matching.bc
// RUN: llvm-link %t.default.bc %t.matching.bc -o %t.good.bc
// RUN: %clang --target=arm64e-apple-macosx15.0 -flto=full -emit-llvm -c %s -DABI_FUNCTION=arm64e_lto_incompatible -fno-ptrauth-vtable-pointer-address-discrimination -o %t.mismatch.bc 2>/dev/null
// RUN: not llvm-link %t.default.bc %t.mismatch.bc -o %t.bad.bc 2>&1 | FileCheck %s --check-prefix=BAD-MERGE
// BAD-MERGE: linking module flags 'arm64e-ptrauth-abi-v1'

// An explicit opt-out is still legal on its own but strict builds must
// discover it rather than silently producing a different LTO contract.
// RUN: not %clang --target=arm64e-apple-macosx15.0 -flto=thin -Werror=arm64e-abi-override -fno-ptrauth-calls -c %s -o %t.strict.bc 2>&1 | FileCheck %s --check-prefix=STRICT
// STRICT: error: option '-fno-ptrauth-calls' changes the arm64e default for indirect calls

#ifndef ABI_FUNCTION
#define ABI_FUNCTION arm64e_lto_anchor
#endif

int ABI_FUNCTION(void) { return 0; }
