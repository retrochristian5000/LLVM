// REQUIRES: aarch64-registered-target
//
// Arm64e remains the effective target even if an explicit -fno-ptrauth
// option disables one of its ABI defaults. Diagnose such overrides instead
// of silently changing the object semantics or prohibiting experiments.

// RUN: %clang -target arm64e-apple-macosx27.2 -c %s -### 2>&1 | FileCheck %s --check-prefix=CLEAN --implicit-check-not='arm64e-abi-override'
// CLEAN: "-cc1" "-triple" "arm64e-apple-macosx27.2.0"
// CLEAN: "-fptrauth-calls"

// RUN: %clang -target arm64e-apple-macosx27.2 -fno-ptrauth-calls -c %s -### 2>&1 | FileCheck %s --check-prefix=CALL
// CALL: warning: option '-fno-ptrauth-calls' changes the arm64e default for indirect calls; verify ABI compatibility [-Warm64e-abi-override]
// CALL: "-cc1"

// RUN: not %clang -target arm64e-apple-macosx27.2 -Werror=arm64e-abi-override -fno-ptrauth-calls -c %s -### 2>&1 | FileCheck %s --check-prefix=STRICT
// STRICT: error: option '-fno-ptrauth-calls' changes the arm64e default for indirect calls; verify ABI compatibility [-Werror,-Warm64e-abi-override]

// RUN: %clang -target arm64e-apple-macosx27.2 -Wno-arm64e-abi-override -fno-ptrauth-calls -c %s -### 2>&1 | FileCheck %s --check-prefix=SUPPRESS --implicit-check-not='warning: option'
// SUPPRESS: "-cc1"

// The last of each positive/negative pair controls the effective ABI.
// RUN: %clang -target arm64e-apple-macosx27.2 -fno-ptrauth-calls -fptrauth-calls -c %s -### 2>&1 | FileCheck %s --check-prefix=RESTORED --implicit-check-not='arm64e-abi-override'
// RESTORED: "-fptrauth-calls"
// RUN: %clang -target arm64e-apple-macosx27.2 -fptrauth-calls -fno-ptrauth-calls -c %s -### 2>&1 | FileCheck %s --check-prefix=CALL

// A plain arm64 compile may use these experimental flags without an arm64e
// ABI warning because it does not advertise an arm64e binary ABI.
// RUN: %clang -target arm64-apple-macosx27.2 -fno-ptrauth-calls -c %s -### 2>&1 | FileCheck %s --check-prefix=PLAIN --implicit-check-not='arm64e-abi-override'
// PLAIN: "-cc1" "-triple" "arm64-apple-macosx27.2.0"

// Warn for the C++ and Objective-C pointer representation overrides as well.
// RUN: %clang -target arm64e-apple-ios18.0 -fno-ptrauth-returns -fno-ptrauth-vtable-pointer-address-discrimination -fno-ptrauth-objc-isa -c %s -### 2>&1 | FileCheck %s --check-prefix=OTHER
// OTHER: warning: option '-fno-ptrauth-returns' changes the arm64e default for return addresses; verify ABI compatibility [-Warm64e-abi-override]
// OTHER: warning: option '-fno-ptrauth-vtable-pointer-address-discrimination' changes the arm64e default for C++ vtable address discrimination; verify ABI compatibility [-Warm64e-abi-override]
// OTHER: warning: option '-fno-ptrauth-objc-isa' changes the arm64e default for Objective-C isa; verify ABI compatibility [-Warm64e-abi-override]

// Other pointer-authentication toggles also affect executable behavior.
// RUN: %clang -target arm64e-apple-macosx27.2 -fno-ptrauth-intrinsics -fno-ptrauth-indirect-gotos -fno-ptrauth-auth-traps -fno-ptrauth-vtable-pointer-type-discrimination -fno-ptrauth-objc-class-ro -fno-ptrauth-objc-interface-sel -c %s -### 2>&1 | FileCheck %s --check-prefix=OTHER-DETAILS
// OTHER-DETAILS: warning: option '-fno-ptrauth-intrinsics' changes the arm64e default for intrinsics; verify ABI compatibility [-Warm64e-abi-override]
// OTHER-DETAILS: warning: option '-fno-ptrauth-indirect-gotos' changes the arm64e default for indirect gotos; verify ABI compatibility [-Warm64e-abi-override]
// OTHER-DETAILS: warning: option '-fno-ptrauth-auth-traps' changes the arm64e default for authentication traps; verify ABI compatibility [-Warm64e-abi-override]
// OTHER-DETAILS: warning: option '-fno-ptrauth-vtable-pointer-type-discrimination' changes the arm64e default for C++ vtable type discrimination; verify ABI compatibility [-Warm64e-abi-override]
// OTHER-DETAILS: warning: option '-fno-ptrauth-objc-class-ro' changes the arm64e default for Objective-C class metadata; verify ABI compatibility [-Warm64e-abi-override]
// OTHER-DETAILS: warning: option '-fno-ptrauth-objc-interface-sel' changes the arm64e default for Objective-C interface selectors; verify ABI compatibility [-Warm64e-abi-override]

// Do not infer Apple branding from the arm64e ABI: Mach-O architecture and
// OS/vendor selection are independent of this effective-flag warning.
