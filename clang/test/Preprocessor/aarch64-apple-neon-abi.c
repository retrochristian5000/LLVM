// REQUIRES: aarch64-registered-target
//
// The arm64e pointer-authentication ABI must not imply NEON availability.
// Apple's legacy __ARM_NEON__ macro must agree with the ACLE __ARM_NEON
// macro and the features used by the generated arm_neon.h.
//
// RUN: %clang --target=arm64-apple-macosx13.0 -mcpu=apple-a18-pro -dM -E -x c %s | FileCheck %s --check-prefix=NEON
// RUN: %clang --target=arm64e-apple-macosx13.0 -mcpu=apple-a18-pro -dM -E -x c %s | FileCheck %s --check-prefixes=NEON,ARM64E
// RUN: %clang --target=arm64-apple-macosx13.0 -mcpu=apple-a18-pro -mgeneral-regs-only -dM -E -x c %s | FileCheck %s --check-prefix=NO-NEON
// RUN: %clang --target=arm64e-apple-macosx13.0 -mcpu=apple-a18-pro -mgeneral-regs-only -dM -E -x c %s | FileCheck %s --check-prefixes=NO-NEON,ARM64E
//
// NEON: #define __ARM_NEON 1
// NEON: #define __ARM_NEON__ 1
// ARM64E: #define __arm64e__ 1
//
// NO-NEON-NOT: #define __ARM_NEON 1
// NO-NEON-NOT: #define __ARM_NEON__ 1
// NO-NEON: #define __arm64__ 1
