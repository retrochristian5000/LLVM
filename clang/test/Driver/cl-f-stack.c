// MSVC /F sets the executable stack reserve (not /GS or /Gs).
// It accepts joined/separate C numeric literals and rounds to four bytes.
// Test all PE architectures, explicit /link precedence and DLL/no-link cases.

// RUN: %clang_cl --target=i686-pc-windows-msvc /Tc%s /F5 -fuse-ld=lld -### 2>&1 | FileCheck %s --check-prefix=ROUND
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /Tc%s /F 5 -fuse-ld=lld -### 2>&1 | FileCheck %s --check-prefix=ROUND
// RUN: %clang_cl --target=aarch64-pc-windows-msvc /Tc%s /F5 -fuse-ld=lld -### 2>&1 | FileCheck %s --check-prefix=ROUND
// ROUND: lld-link
// ROUND: "-stack:8"

// RUN: %clang_cl --target=i686-pc-windows-msvc /Tc%s /F010 -fuse-ld=lld -### 2>&1 | FileCheck %s --check-prefix=OCTAL
// OCTAL: "-stack:8"
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /Tc%s /F0x4001 -fuse-ld=lld -### 2>&1 | FileCheck %s --check-prefix=HEX
// HEX: "-stack:16388"
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /Tc%s /F12345 -fuse-ld=lld -### 2>&1 | FileCheck %s --check-prefix=DECIMAL
// DECIMAL: "-stack:12348"

// RUN: %clang_cl --target=x86_64-pc-windows-msvc /Tc%s /F8 /F27 -fuse-ld=lld -### 2>&1 | FileCheck %s --check-prefix=LAST
// LAST: "-stack:28"
// LAST-NOT: "-stack:8"
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /Tc%s /F5 -fuse-ld=lld -### /link /STACK:65536 2>&1 | FileCheck %s --check-prefix=OVERRIDE
// OVERRIDE: "-stack:8"
// OVERRIDE: "/STACK:65536"

// RUN: %clang_cl --target=x86_64-pc-windows-msvc /Tc%s /LD /F5 -fuse-ld=lld -### 2>&1 | FileCheck %s --check-prefix=DLL
// DLL: lld-link
// DLL-NOT: "-stack:8"

// RUN: not %clang_cl --target=i686-pc-windows-msvc /Tc%s /F0 -fuse-ld=lld -### 2>&1 | FileCheck %s --check-prefix=ZERO
// ZERO: error: invalid value '0'
// RUN: not %clang_cl --target=x86_64-pc-windows-msvc /Tc%s /Fgarbage -fuse-ld=lld -### 2>&1 | FileCheck %s --check-prefix=INVALID
// INVALID: error: invalid value 'garbage'
// RUN: not %clang_cl --target=i686-pc-windows-msvc /Tc%s /F4294967293 -fuse-ld=lld -### 2>&1 | FileCheck %s --check-prefix=PE32-LIMIT
// PE32-LIMIT: error: invalid value '4294967293'
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /Tc%s /F4294967293 -fuse-ld=lld -### 2>&1 | FileCheck %s --check-prefix=PE32PLUS
// PE32PLUS: "-stack:4294967296"

int main(void) { return 0; }
