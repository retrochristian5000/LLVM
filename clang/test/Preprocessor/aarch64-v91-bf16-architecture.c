// RUN: %clang --target=aarch64-none-linux-gnu -march=armv8.5-a -E -dM %s | FileCheck %s --check-prefix=V85 --implicit-check-not=__ARM_FEATURE_BF16 --implicit-check-not=__ARM_FEATURE_MATMUL_INT8
// RUN: %clang --target=aarch64-none-linux-gnu -march=armv8.6-a -E -dM %s | FileCheck %s --check-prefix=V86
// RUN: %clang --target=aarch64-none-linux-gnu -march=armv9-a -E -dM %s | FileCheck %s --check-prefix=V90 --implicit-check-not=__ARM_FEATURE_BF16 --implicit-check-not=__ARM_FEATURE_MATMUL_INT8
// RUN: %clang --target=aarch64-none-linux-gnu -march=armv9.1-a -E -dM %s | FileCheck %s --check-prefix=BF16V9
// RUN: %clang --target=aarch64-none-linux-gnu -march=armv9.2-a -E -dM %s | FileCheck %s --check-prefix=BF16V9
//
// BF16 and I8MM become architecture requirements at Armv8.6-A/Armv9.1-A.
// Armv9.0-A does not imply either one. Verify the Clang preprocessor feature
// path across the version boundary rather than adding unconditional macros.
//
// V85: #define __ARM_ARCH 8
// V86-DAG: #define __ARM_ARCH 8
// V86-DAG: #define __ARM_BF16_FORMAT_ALTERNATIVE 1
// V86-DAG: #define __ARM_FEATURE_BF16 1
// V86-DAG: #define __ARM_FEATURE_BF16_SCALAR_ARITHMETIC 1
// V86-DAG: #define __ARM_FEATURE_BF16_VECTOR_ARITHMETIC 1
// V86-DAG: #define __ARM_FEATURE_MATMUL_INT8 1
// V90: #define __ARM_ARCH 9
// BF16V9-DAG: #define __ARM_ARCH 9
// BF16V9-DAG: #define __ARM_BF16_FORMAT_ALTERNATIVE 1
// BF16V9-DAG: #define __ARM_FEATURE_BF16 1
// BF16V9-DAG: #define __ARM_FEATURE_BF16_SCALAR_ARITHMETIC 1
// BF16V9-DAG: #define __ARM_FEATURE_BF16_VECTOR_ARITHMETIC 1
// BF16V9-DAG: #define __ARM_FEATURE_MATMUL_INT8 1
