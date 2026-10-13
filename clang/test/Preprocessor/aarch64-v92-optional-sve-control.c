// RUN: %clang --target=aarch64-none-linux-gnu -mcpu=neoverse-n3 -E -dM %s | FileCheck %s --check-prefix=V92-SVE
// RUN: %clang --target=aarch64-none-linux-gnu -march=armv9.2-a -E -dM %s | FileCheck %s --check-prefix=V92-SVE
//
// Keep the CPU-defined SVE/SVE2 capabilities of processors that implement
// them.  Splitting architecture identity from optional extensions must not
// disable extensions requested by a named CPU or the generic -march defaults.
//
// V92-SVE-DAG: #define __ARM_ARCH 9
// V92-SVE-DAG: #define __ARM_FEATURE_SVE 1
// V92-SVE-DAG: #define __ARM_FEATURE_SVE2 1
