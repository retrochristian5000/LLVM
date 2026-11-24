// RUN: %clangxx -target arm64e-apple-macosx13.0 -march=armv9.2-a -DEXPECTED=2 -fsyntax-only %s
// RUN: %clangxx -target arm64-apple-macosx13.0 -mcpu=apple-a18-pro -DEXPECTED=2 -DAPPLE_CPU -fsyntax-only %s
// RUN: %clangxx -target aarch64-linux-gnu -march=armv8.2-a -DEXPECTED=0 -fsyntax-only %s
// RUN: %clangxx -target aarch64-linux-gnu -march=armv8.2-a+rcpc -DEXPECTED=1 -fsyntax-only %s
// RUN: %clangxx -target aarch64-linux-gnu -march=armv8.4-a -DEXPECTED=2 -fsyntax-only %s
// RUN: %clangxx -target aarch64-linux-gnu -march=armv9.2-a -DEXPECTED=2 -fsyntax-only %s
// RUN: %clangxx -target aarch64-linux-gnu -march=armv8.2-a+rcpc3 -DEXPECTED=3 -fsyntax-only %s
// RUN: %clangxx -target aarch64-linux-gnu -march=armv9.2-a+norcpc -DEXPECTED=0 -fsyntax-only %s
// RUN: %clang_cc1 -triple arm64-apple-macosx13.0 -target-feature +v9.2a -target-feature +rcpc -target-feature -rcpc-immo -DEXPECTED=1 -fsyntax-only %s

#ifndef __ARM_FEATURE_RCPC
#define RCPC_LEVEL 0
#else
#define RCPC_LEVEL __ARM_FEATURE_RCPC
#endif

static_assert(RCPC_LEVEL == EXPECTED, "Incorrect ACLE RCpc feature level");

#if defined(APPLE_CPU) && (defined(__ARM_FEATURE_SVE) || defined(__ARM_FEATURE_SVE2))
#error "Apple CPU selection must not inherit generic Armv9 SVE defaults"
#endif
