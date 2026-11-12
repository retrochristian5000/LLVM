// REQUIRES: aarch64-registered-target
//
// A generated arm_neon.h must use the selected compilation target's feature
// model, not the build machine's CPU. Tuning for A18 Pro alone must not
// introduce A18 Pro instructions into the portable Armv8-A baseline.
//
// RUN: %clang --target=arm64-apple-macosx13.0 -mcpu=apple-a18-pro -ffreestanding -fsyntax-only -std=c11 --sysroot=%S/Inputs -DTEST_A18_PRO %s
// RUN: %clang --target=arm64-apple-macosx13.0 -march=armv8-a -mtune=apple-a18-pro -ffreestanding -fsyntax-only -std=c11 --sysroot=%S/Inputs -DTEST_TUNE_ONLY %s

#include <arm_neon.h>

#ifndef __ARM_NEON
#error "AArch64 NEON was not enabled"
#endif

#if defined(TEST_A18_PRO)
#ifndef __ARM_FEATURE_BF16
#error "Apple A18 Pro must expose its configured BF16 feature"
#endif
#ifndef __ARM_FEATURE_SME2
#error "Apple A18 Pro must expose its configured SME2 feature"
#endif
#ifdef __ARM_FEATURE_SVE
#error "Apple A18 Pro must not inherit optional non-streaming SVE"
#endif
#ifdef __ARM_FEATURE_SVE2
#error "Apple A18 Pro must not inherit optional non-streaming SVE2"
#endif

float32x4_t a18_pro_bf16_dot(float32x4_t sum, bfloat16x8_t left,
                            bfloat16x8_t right)
{
  return vbfdotq_f32(sum, left, right);
}
#endif

#if defined(TEST_TUNE_ONLY)
#ifdef __ARM_FEATURE_BF16
#error "A18 Pro tuning must not enable BF16 on an Armv8-A baseline"
#endif

uint8x16_t a18_pro_tuned_neon_add(uint8x16_t left, uint8x16_t right)
{
  return vaddq_u8(left, right);
}
#endif
