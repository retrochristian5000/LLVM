// RUN: %clang --target=arm64-apple-macosx11.0 -mcpu=apple-a18 -E -dM %s | FileCheck %s --check-prefix=A18 --implicit-check-not=__ARM_FEATURE_SVE
// RUN: %clang --target=arm64-apple-macosx11.0 -mcpu=apple-m4 -E -dM %s | FileCheck %s --check-prefix=A18 --implicit-check-not=__ARM_FEATURE_SVE
//
// A18 Pro and M4 are ISA-feature aliases in LLVM. The Armv9.2-A
// architectural spelling must not cause us to infer SVE/SVE2 on Apple
// cores that omit SVE but implement selected later instructions.
//
// Test the actual Clang preprocessor path, not only TableGen CPU aliases.
// A18-DAG: #define __ARM_FEATURE_BF16 1
// A18-DAG: #define __ARM_FEATURE_DOTPROD 1
// A18-DAG: #define __ARM_FEATURE_MATMUL_INT8 1
