// Ensure MSVC /arch on ARM64/ARM64EC selects AArch64 ISA features rather
// than silently discarding them or enabling x86 AVX target features.
// This option never changes the ARM64EC calling convention or COFF ABI.
//
// RUN: %clang_cl --target=aarch64-pc-windows-msvc /arch:armv8.2+lse /c -### -- %s 2>&1 | FileCheck %s --check-prefix=ARM64-82
// ARM64-82: "-triple" "aarch64-pc-windows-msvc{{[^"]*}}"
// ARM64-82-SAME: "-target-feature" "+v8.2a"
// ARM64-82-SAME: "-target-feature" "+lse"
//
// RUN: %clang_cl --target=arm64ec-pc-windows-msvc /arch:armv8.2+lse /c -### -- %s 2>&1 | FileCheck %s --check-prefix=EC-82
// EC-82: "-triple" "arm64ec-pc-windows-msvc{{[^"]*}}"
// EC-82-SAME: "-target-feature" "+v8.2a"
// EC-82-SAME: "-target-feature" "+lse"
// EC-82-NOT: "+avx10.2"
// EC-82-NOT: "-mprefer-vector-width"
//
// RUN: %clang_cl --target=arm64ec-pc-windows-msvc /arch:armv8.0 /c -### -- %s 2>&1 | FileCheck %s --check-prefix=EC-80
// EC-80: "-target-feature" "+v8a"
//
// RUN: %clang_cl --target=arm64ec-pc-windows-msvc /arch:armv9.4 /c -### -- %s 2>&1 | FileCheck %s --check-prefix=EC-94
// EC-94: "-target-feature" "+v9.4a"
//
// RUN: not %clang_cl --target=arm64ec-pc-windows-msvc /arch:AVX10.2 /c -### -- %s 2>&1 | FileCheck %s --check-prefix=EC-X86
// EC-X86: error: unsupported argument
// EC-X86-SAME: option '/arch:'
//
// RUN: not %clang_cl --target=aarch64-pc-windows-msvc /arch:armv8.10 /c -### -- %s 2>&1 | FileCheck %s --check-prefix=ARM-BAD
// ARM-BAD: error: unsupported argument
// ARM-BAD-SAME: option '/arch:'
//
// RUN: %clang_cl --target=arm64ec-pc-windows-msvc /arch:armv8.2+lse -march=armv8-a /c -### -- %s 2>&1 | FileCheck %s --check-prefix=EC-OVERRIDE
// EC-OVERRIDE: "-target-feature" "+v8a"
// EC-OVERRIDE-NOT: "-target-feature" "+v8.2a"
//
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /arch:AVX10.2 /c -### -- %s 2>&1 | FileCheck %s --check-prefix=X64-AVX10
// X64-AVX10: "-target-cpu" "sapphirerapids"
// X64-AVX10-SAME: "-target-feature" "-amx-tile"
// X64-AVX10-SAME: "-target-feature" "+avx10.2"
int test(int value) { return value + 1; }
