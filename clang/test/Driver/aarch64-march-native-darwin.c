// REQUIRES: system-darwin,aarch64-host,aarch64-registered-target
//
// AArch64 macOS guarantees FP/AdvSIMD in the host ISA. `-march=native`
// must retain them even on macOS revisions that omit optional-feature
// sysctl keys. This test exercises the *native* Clang driver host-feature
// path, not a synthetic feature list.
//
// RUN: %clang --target=arm64-apple-macosx -march=native -### -c %s 2>&1 | FileCheck %s --check-prefix=NATIVE
// RUN: %clang --target=arm64-apple-macosx -mcpu=native -### -c %s 2>&1 | FileCheck %s --check-prefix=NATIVE
// RUN: %clang --target=arm64-apple-macosx -mtune=native -### -c %s 2>&1 | FileCheck %s --check-prefix=TUNE
// TUNE-NOT: error:
// TUNE: "-cc1"{{.*}} "-tune-cpu" "{{[^"]+}}"
// TUNE-NOT: error:
// NATIVE-NOT: error:
// NATIVE: "-cc1"{{.*}} "-target-cpu" "{{[^"]+}}"
// NATIVE: "-target-feature" "+fp-armv8"
// NATIVE: "-target-feature" "+neon"
// NATIVE-NOT: error:
//
// The test checks host ISA selection only; it does not imply `arm64e`
// pointer authentication or switch the Mach-O subtype.
