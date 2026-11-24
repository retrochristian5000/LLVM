// REQUIRES: x86-registered-target, aarch64-registered-target
// RUN: %clang -target x86_64-apple-macosx13.0 -pie %s -### 2>&1 | FileCheck %s --check-prefix=PIE
// RUN: %clang -target x86_64-apple-macosx13.0 -no-pie %s -### 2>&1 | FileCheck %s --check-prefix=NOPIE
// RUN: %clang -target x86_64-apple-macosx13.0 -fPIE -no-pie %s -### 2>&1 | FileCheck %s --check-prefix=NOPIE
// RUN: %clang -target x86_64-apple-macosx13.0 -fno-PIE -pie %s -### 2>&1 | FileCheck %s --check-prefix=PIE
// RUN: %clang -target x86_64-apple-macosx13.0 -no-pie -pie %s -### 2>&1 | FileCheck %s --check-prefix=PIE
// RUN: %clang -target arm64e-apple-macosx13.0 -fPIE -pie %s -### 2>&1 | FileCheck %s --check-prefix=ARM64E
// RUN: %clang -target arm64-apple-macosx13.0 -dynamiclib -fPIE %s -### 2>&1 | FileCheck %s --check-prefix=DYLIB
// RUN: %clang -target x86_64-apple-macosx13.0 -bundle -fPIE %s -### 2>&1 | FileCheck %s --check-prefix=BUNDLE
//
// PIE-NOT: "-no_pie"
// PIE: "-pie"
// PIE-NOT: "-no_pie"
// NOPIE-NOT: "-pie"
// NOPIE: "-no_pie"
// NOPIE-NOT: "-pie"
// ARM64E: "-cc1" "-triple" "arm64e-apple-macosx13.0.0"
// ARM64E: "-mrelocation-model" "pic"
// ARM64E: "-arch" "arm64e"
// ARM64E: "-pie"
// DYLIB: "-dylib"
// DYLIB-NOT: "-pie"
// DYLIB-NOT: "-no_pie"
// BUNDLE: "-bundle"
// BUNDLE-NOT: "-pie"
// BUNDLE-NOT: "-no_pie"

int main(void) { return 0; }
