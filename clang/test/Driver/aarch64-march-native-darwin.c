// REQUIRES: system-darwin,aarch64-host
//
// AArch64 macOS guarantees FP/AdvSIMD in the host ISA. `-march=native`
// must retain them even on macOS revisions that omit optional-feature
// sysctl keys. This test exercises the *native* Clang driver host-feature
// path, not a synthetic feature list.
//
// RUN: %clang --target=arm64-apple-macosx -march=native -### -c %s 2>&1 | FileCheck %s --check-prefix=NATIVE
// RUN: %clang --target=arm64-apple-macosx -mcpu=native -### -c %s 2>&1 | FileCheck %s --check-prefix=NATIVE
// NATIVE: "-cc1"{{.*}} "-target-cpu" "{{[^"]+}}"
// NATIVE: "-target-feature" "+fp-armv8"
// NATIVE: "-target-feature" "+neon"
//
// The test checks host ISA selection only; it does not imply `arm64e`
// pointer authentication or switch the Mach-O subtype.
