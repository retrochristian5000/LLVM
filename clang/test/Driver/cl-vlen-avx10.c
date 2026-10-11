// Microsoft's /vlen is an optimization-width preference, not an /arch
// replacement. On AVX10.x, /vlen used to skip +avx10.2 and -amx-tile
// target features because both lived in the no-/vlen branch.
// Verify the selected ISA is preserved for bare, 256-bit and 512-bit
// forms on both x86 and x64 Windows targets.
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /arch:AVX10.2 /vlen=256 -### -- %s 2>&1 | FileCheck %s --check-prefix=AVX102-256
// AVX102-256: "-mprefer-vector-width=256" "-target-feature" "-amx-tile" "-target-feature" "+avx10.2"
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /arch:AVX10.2 /vlen=512 -### -- %s 2>&1 | FileCheck %s --check-prefix=AVX102-512
// AVX102-512: "-mprefer-vector-width=512" "-target-feature" "-amx-tile" "-target-feature" "+avx10.2"
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /arch:AVX10.2 /vlen -### -- %s 2>&1 | FileCheck %s --check-prefix=AVX102-DEFAULT
// AVX102-DEFAULT: "-mprefer-vector-width=256" "-target-feature" "-amx-tile" "-target-feature" "+avx10.2"
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /arch:AVX10.2 -### -- %s 2>&1 | FileCheck %s --check-prefix=AVX102-NONE
// AVX102-NONE: "-mprefer-vector-width=256" "-target-feature" "-amx-tile" "-target-feature" "+avx10.2"
// RUN: %clang_cl --target=i686-pc-windows-msvc /arch:AVX10.2 /vlen=512 -### -- %s 2>&1 | FileCheck %s --check-prefix=AVX102-X86
// AVX102-X86: "-mprefer-vector-width=512" "-target-feature" "-amx-tile" "-target-feature" "+avx10.2"

// /arch:AVX10.1 never gains AVX10.2 instructions from /vlen.
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /arch:AVX10.1 /vlen=512 -### -- %s 2>&1 | FileCheck %s --check-prefix=AVX101-512 --implicit-check-not="+avx10.2"
// AVX101-512: "-mprefer-vector-width=512" "-target-feature" "-amx-tile"
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /arch:AVX10.1 /vlen -### -- %s 2>&1 | FileCheck %s --check-prefix=AVX101-DEFAULT --implicit-check-not="+avx10.2"
// AVX101-DEFAULT: "-mprefer-vector-width=256" "-target-feature" "-amx-tile"
// RUN: %clang_cl --target=i686-pc-windows-msvc /arch:AVX10.1 /vlen=256 -### -- %s 2>&1 | FileCheck %s --check-prefix=AVX101-X86 --implicit-check-not="+avx10.2"
// AVX101-X86: "-mprefer-vector-width=256" "-target-feature" "-amx-tile"

// Negative controls: AVX512's vector preference should not imply AVX10,
// and unsupported 512-bit preference on AVX2 must still issue a warning.
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /arch:AVX512 /vlen=256 -### -- %s 2>&1 | FileCheck %s --check-prefix=AVX512 --implicit-check-not="+avx10.2"
// AVX512: "-mprefer-vector-width=256"
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /arch:AVX2 /vlen=512 -### -- %s 2>&1 | FileCheck %s --check-prefix=AVX2-WARN --implicit-check-not="+avx10.2"
// AVX2-WARN: warning: invalid argument '/vlen=512' not allowed with '/arch:AVX2'

void fixture(void) {}
