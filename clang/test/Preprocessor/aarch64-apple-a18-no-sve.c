// RUN: %clang --target=arm64-apple-macosx11.0 -mcpu=apple-a18 -E -dM %s | FileCheck %s --check-prefix=A18 --implicit-check-not=__ARM_FEATURE_SVE
// RUN: %clang --target=arm64-apple-macosx11.0 -mcpu=apple-a18-pro -E -dM %s | FileCheck %s --check-prefix=A18 --implicit-check-not=__ARM_FEATURE_SVE
// RUN: %clang --target=arm64-apple-macosx11.0 -mcpu=apple-m4 -E -dM %s | FileCheck %s --check-prefix=A18 --implicit-check-not=__ARM_FEATURE_SVE
//
// A18, A18 Pro and M4 are distinct CPUs with conservatively shared
// Armv9.2-A features, without implicit non-streaming SVE/SVE2.
//
// Test the actual Clang preprocessor path, not only TableGen CPU aliases.
// A18-DAG: #define __ARM_ARCH 9
// A18-DAG: #define __ARM_FEATURE_SME 1
// A18-DAG: #define __ARM_FEATURE_SME2 1
// A18-DAG: #define __ARM_FEATURE_BF16 1
// A18-DAG: #define __ARM_FEATURE_DOTPROD 1
// A18-DAG: #define __ARM_FEATURE_MATMUL_INT8 1
