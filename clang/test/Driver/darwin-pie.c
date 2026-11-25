// REQUIRES: x86-registered-target, aarch64-registered-target
// RUN: %clang -target x86_64-apple-macosx13.0 -pie %s -### 2>&1 | FileCheck %s --check-prefix=PIE
// RUN: %clang -target x86_64-apple-macosx13.0 -no-pie %s -### 2>&1 | FileCheck %s --check-prefix=NOPIE
// RUN: %clang -target x86_64-apple-macosx13.0 -fPIE -no-pie %s -### 2>&1 | FileCheck %s --check-prefix=NOPIE
// RUN: %clang -target x86_64-apple-macosx13.0 -fno-PIE -pie %s -### 2>&1 | FileCheck %s --check-prefix=PIE
// RUN: %clang -target x86_64-apple-macosx13.0 -no-pie -pie %s -### 2>&1 | FileCheck %s --check-prefix=PIE
// RUN: %clang -target arm64e-apple-macosx13.0 -fPIE -pie %s -### 2>&1 | FileCheck %s --check-prefix=ARM64E
// RUN: %clang -target arm64-apple-macosx13.0 -dynamiclib -fPIE %s -### 2>&1 | FileCheck %s --check-prefix=DYLIB
// RUN: %clang -target x86_64-apple-macosx13.0 -bundle -fPIE %s -### 2>&1 | FileCheck %s --check-prefix=BUNDLE
// RUN: %clang -target arm64-apple-macosx13.0 -Werror -fPIE -pie %s -### 2>&1 | FileCheck %s --check-prefix=PIE --implicit-check-not='error:' --implicit-check-not='warning:'
// RUN: %clangxx -target arm64e-apple-macosx13.0 -Werror -fPIE -pie -x c++ %s -### 2>&1 | FileCheck %s --check-prefix=ARM64E --implicit-check-not='error:' --implicit-check-not='warning:'
// RUN: %clang -target arm64-apple-macosx13.0 -Werror -fPIE -pie -x objective-c %s -### 2>&1 | FileCheck %s --check-prefix=PIE --implicit-check-not='error:' --implicit-check-not='warning:'
// RUN: %clang -target x86_64-apple-macosx13.0 -fPIE -fno-PIE %s -### 2>&1 | FileCheck %s --check-prefix=NOPIE
// RUN: %clang -target x86_64-apple-macosx13.0 -fno-PIE -fpie %s -### 2>&1 | FileCheck %s --check-prefix=PIE
// RUN: %clang -target x86_64-apple-macosx13.0 -fpie -r -nostdlib %s -### 2>&1 | FileCheck %s --check-prefix=RELOC --implicit-check-not='"-pie"' --implicit-check-not='"-no_pie"'
// RUN: %clang -target x86_64-apple-macosx13.0 -fno-PIE -r -nostdlib %s -### 2>&1 | FileCheck %s --check-prefix=RELOC --implicit-check-not='"-pie"' --implicit-check-not='"-no_pie"'
// RUN: %clang -target arm64-apple-macosx13.0 -pie -r -nostdlib %s -### 2>&1 | FileCheck %s --check-prefix=RELOC --implicit-check-not='"-pie"' --implicit-check-not='"-no_pie"'
// RUN: not %clang -target arm64-apple-macosx13.0 -static-pie %s -### 2>&1 | FileCheck %s --check-prefix=STATIC-PIE
// RUN: not %clang -target x86_64-apple-macosx13.0 -static-pie %s -### 2>&1 | FileCheck %s --check-prefix=STATIC-PIE
// RUN: %clang -target arm64e-apple-macosx13.0 -fno-PIE -c %s -### 2>&1 | FileCheck %s --check-prefix=FORCED-PIC
//
// RELOC: "-r"
// STATIC-PIE: error: unsupported option '-static-pie' for target
// FORCED-PIC: "-mrelocation-model" "pic"
// FORCED-PIC-SAME: "-pic-level" "2"
// FORCED-PIC-NOT: "-pic-is-pie"
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
