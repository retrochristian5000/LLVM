// MSVC treats /GS security cookies and /Gs stack probing independently.
// Bare /Gs and /Gs0 are target-dependent; 1-9 warn and have the same effect.
// Values 2147483648+ are fatal in MSVC.
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /c /Gs -### %s 2>&1 | FileCheck %s --check-prefix=X64-BARE
// X64-BARE: "-mstack-probe-size=0"
// RUN: %clang_cl --target=i686-pc-windows-msvc /c /Gs -### %s 2>&1 | FileCheck %s --check-prefix=X86-BARE
// X86-BARE: "-mstack-probe-size=4096"
// RUN: %clang_cl --target=aarch64-pc-windows-msvc /c /Gs -### %s 2>&1 | FileCheck %s --check-prefix=ARM64-BARE
// ARM64-BARE: "-mstack-probe-size=4096"

// RUN: %clang_cl --target=x86_64-pc-windows-msvc /c /Gs0 -### %s 2>&1 | FileCheck %s --check-prefix=X64-ZERO
// X64-ZERO: "-mstack-probe-size=0"
// RUN: %clang_cl --target=i686-pc-windows-msvc /c /Gs0 -### %s 2>&1 | FileCheck %s --check-prefix=X86-ZERO
// X86-ZERO: "-mstack-probe-size=4096"
// RUN: %clang_cl --target=aarch64-pc-windows-msvc /c /Gs0 -### %s 2>&1 | FileCheck %s --check-prefix=ARM64-ZERO
// ARM64-ZERO: "-mstack-probe-size=4096"

// RUN: %clang_cl --target=x86_64-pc-windows-msvc /c /Gs1 -### %s 2>&1 | FileCheck %s --check-prefix=X64-SMALL
// X64-SMALL: warning: /Gs1 is too small; using stack probe threshold 0 for this target
// X64-SMALL: "-mstack-probe-size=0"
// RUN: %clang_cl --target=i686-pc-windows-msvc /c /Gs9 -### %s 2>&1 | FileCheck %s --check-prefix=X86-SMALL
// X86-SMALL: warning: /Gs9 is too small; using stack probe threshold 4096 for this target
// X86-SMALL: "-mstack-probe-size=4096"
// RUN: %clang_cl --target=aarch64-pc-windows-msvc /c /Gs1 -### %s 2>&1 | FileCheck %s --check-prefix=ARM64-SMALL
// ARM64-SMALL: warning: /Gs1 is too small; using stack probe threshold 4096 for this target
// ARM64-SMALL: "-mstack-probe-size=4096"

// RUN: %clang_cl --target=x86_64-pc-windows-msvc /c /Gs10 -### %s 2>&1 | FileCheck %s --check-prefix=TEN
// TEN: "-mstack-probe-size=10"
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /c /Gs2147483647 -### %s 2>&1 | FileCheck %s --check-prefix=MAX
// MAX: "-mstack-probe-size=2147483647"
// RUN: not %clang_cl --target=x86_64-pc-windows-msvc /c /Gs2147483648 -### %s 2>&1 | FileCheck %s --check-prefix=TOO-LARGE
// TOO-LARGE: error: invalid value '2147483648' in '/Gs2147483648'
// RUN: not %clang_cl --target=x86_64-pc-windows-msvc /c /Gsabc -### %s 2>&1 | FileCheck %s --check-prefix=NOT-NUMERIC
// NOT-NUMERIC: error: invalid value 'abc' in '/Gsabc'

// /GS- does not disable /Gs. /GS enables stack cookies independently.
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /c /GS- /Gs0 -### %s 2>&1 | FileCheck %s --check-prefix=NO-COOKIE
// NO-COOKIE-NOT: "-stack-protector"
// NO-COOKIE: "-mstack-probe-size=0"
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /c /GS /Gs8192 -### %s 2>&1 | FileCheck %s --check-prefix=BOTH
// BOTH: "-stack-protector" "2"
// BOTH: "-mstack-probe-size=8192"

// Last stack-probe option wins, but /GS continues to be independent.
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /c /Gs8192 /Gs -### %s 2>&1 | FileCheck %s --check-prefix=LAST-BARE
// LAST-BARE: "-mstack-probe-size=0"
// RUN: %clang_cl --target=x86_64-pc-windows-msvc /c /Gs /Gs8192 -### %s 2>&1 | FileCheck %s --check-prefix=LAST-VALUE
// LAST-VALUE: "-mstack-probe-size=8192"

int stack_flags_control;
