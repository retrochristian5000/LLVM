// Check the generated <arm_neon.h> using Apple AArch64 target ABI modes.
// arm64e is a macOS ABI variant, not Windows ARM64EC. Both use AArch64
// AdvSIMD types; the installed Clang resource header must be available in
// freestanding builds without importing host SDK headers.
//
// RUN: %clang -target arm64-apple-macosx14.0 -ffreestanding -fsyntax-only -Wall -Werror %s
// RUN: %clang -target arm64e-apple-macosx14.0 -ffreestanding -fsyntax-only -Wall -Werror %s
// RUN: %clang -target arm64-apple-ios17.0 -ffreestanding -fsyntax-only -Wall -Werror %s
// RUN: %clang -target arm64e-apple-macosx14.0 -ffreestanding -S -emit-llvm -o /dev/null %s
// REQUIRES: aarch64-registered-target

#include <arm_neon.h>

_Static_assert(sizeof(uint8x8_t) == 8, "NEON D register must be 64-bit");
_Static_assert(sizeof(uint8x16_t) == 16, "NEON Q register must be 128-bit");
_Static_assert(sizeof(float32x4_t) == 16, "NEON float vector size changed");
_Static_assert(__alignof__(uint8x16_t) == 16, "NEON vector alignment changed");

uint32x4_t neon_apple_roundtrip(const uint32_t *source, uint32_t *destination,
                                uint32x4_t value) {
  uint32x4_t loaded = vld1q_u32(source);
  uint32x4_t result = vaddq_u32(loaded, value);
  vst1q_u32(destination, result);
  return result;
}
