# REQUIRES: aarch64, x86

# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-darwin %s -o %t.x86_64.o
# RUN: llvm-mc -filetype=obj -triple=arm64-apple-darwin %s -o %t.arm64.o
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-darwin %s -o %t.arm64e.o
# RUN: llvm-mc -filetype=obj -triple=arm64_32-apple-watchos %s -o %t.arm64_32.o

# RUN: %lld -arch x86_64 -no_pie -o %t %t.x86_64.o
# RUN: llvm-readobj --file-headers %t | FileCheck %s --check-prefix=EXEC --implicit-check-not=MH_PIE
# RUN: %lld -arch x86_64 -o %t %t.x86_64.o
# RUN: llvm-readobj --file-headers %t | FileCheck %s --check-prefix=PIE
# RUN: %lld -arch x86_64 -no_pie -pie -o %t %t.x86_64.o
# RUN: llvm-readobj --file-headers %t | FileCheck %s --check-prefix=PIE
# RUN: %lld -arch x86_64 -pie -no_pie -o %t %t.x86_64.o
# RUN: llvm-readobj --file-headers %t | FileCheck %s --check-prefix=EXEC --implicit-check-not=MH_PIE
# RUN: %lld -arch arm64 -pie -o %t %t.arm64.o
# RUN: llvm-readobj --file-headers %t | FileCheck %s --check-prefix=PIE
# RUN: %no-fatal-warnings-lld -arch arm64 -no_pie -o %t %t.arm64.o 2>&1 | FileCheck %s --check-prefix=ARM64-WARN
# RUN: llvm-readobj --file-headers %t | FileCheck %s --check-prefix=PIE
# RUN: not %lld -arch arm64 -no_pie -o %t %t.arm64.o 2>&1 | FileCheck %s --check-prefix=ARM64
# RUN: not %lld -arch arm64e -no_pie -o %t %t.arm64e.o 2>&1 | FileCheck %s --check-prefix=ARM64E
# RUN: not %lld-watchos -arch arm64_32 -no_pie -o %t %t.arm64_32.o 2>&1 | FileCheck %s --check-prefix=ARM64-32
# RUN: %lld -arch arm64 -dylib -no_pie -o %t %t.arm64.o
# RUN: llvm-readobj --file-headers %t | FileCheck %s --check-prefix=DYLIB --implicit-check-not=MH_PIE
# RUN: %lld -arch arm64 -bundle -no_pie -o %t %t.arm64.o
# RUN: llvm-readobj --file-headers %t | FileCheck %s --check-prefix=BUNDLE --implicit-check-not=MH_PIE

# EXEC: FileType: Executable
# PIE: FileType: Executable
# PIE: MH_PIE
# DYLIB: FileType: DynamicLibrary
# BUNDLE: FileType: Bundle
# ARM64-WARN: warning: -no_pie ignored for arm64
# ARM64: error: -no_pie ignored for arm64{{$}}
# ARM64E: error: -no_pie ignored for arm64e{{$}}
# ARM64-32: error: -no_pie ignored for arm64_32{{$}}

.globl _main
_main:
  ret
