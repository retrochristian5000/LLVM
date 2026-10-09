# REQUIRES: aarch64

# AArch64 ADRP relocations use 4 KiB pages.  The PE section layout must
# remain page-aligned even if /driver asks for a smaller SectionAlignment.
# Exercise inferred /machine and both hybrid machine personalities, too.

# RUN: llvm-mc -filetype=obj -triple=aarch64-windows %s -o %t.arm64.obj
# RUN: llvm-mc -filetype=obj -triple=arm64ec-windows %s -o %t.arm64ec.obj

# RUN: not lld-link /machine:arm64 /entry:main /subsystem:console /align:512 /out:%t.exe %t.arm64.obj 2>&1 | FileCheck %s --check-prefix=ARM64
# RUN: not lld-link /entry:main /subsystem:console /align:2048 /out:%t.exe %t.arm64.obj 2>&1 | FileCheck %s --check-prefix=INFERRED
# RUN: not lld-link /machine:arm64 /entry:main /subsystem:console /driver /align:32 /out:%t.exe %t.arm64.obj 2>&1 | FileCheck %s --check-prefix=ARM64-DRIVER
# RUN: not lld-link /machine:arm64ec /dll /noentry /subsystem:console /align:512 /out:%t.dll %t.arm64ec.obj 2>&1 | FileCheck %s --check-prefix=ARM64EC
# RUN: not lld-link /machine:arm64x /dll /noentry /subsystem:console /align:512 /out:%t.dll %t.arm64.obj 2>&1 | FileCheck %s --check-prefix=ARM64X

# ARM64: error: /align:512 is too small for arm64 (AArch64 PE images require at least 4096-byte section alignment)
# INFERRED: error: /align:2048 is too small for arm64 (AArch64 PE images require at least 4096-byte section alignment)
# ARM64-DRIVER: error: /align:32 is too small for arm64 (AArch64 PE images require at least 4096-byte section alignment)
# ARM64EC: error: /align:512 is too small for arm64ec (AArch64 PE images require at least 4096-byte section alignment)
# ARM64X: error: /align:512 is too small for arm64x (AArch64 PE images require at least 4096-byte section alignment)

# FileAlignment must never exceed SectionAlignment for ARM64 images.
# A 4 KiB section size alone is not enough when /filealign is larger.
# RUN: not lld-link /machine:arm64 /entry:main /subsystem:console /filealign:8192 /out:%t.exe %t.arm64.obj 2>&1 | FileCheck %s --check-prefix=ARM64-FILEALIGN
# RUN: not lld-link /entry:main /subsystem:console /align:4096 /filealign:8192 /out:%t.exe %t.arm64.obj 2>&1 | FileCheck %s --check-prefix=INFERRED-FILEALIGN
# RUN: not lld-link /machine:arm64ec /dll /noentry /filealign:8192 /out:%t.dll %t.arm64ec.obj 2>&1 | FileCheck %s --check-prefix=ARM64EC-FILEALIGN
# RUN: not lld-link /machine:arm64x /dll /noentry /filealign:8192 /out:%t.dll %t.arm64.obj 2>&1 | FileCheck %s --check-prefix=ARM64X-FILEALIGN
# ARM64-FILEALIGN: error: /filealign:8192 exceeds /align:4096 for arm64 (PE FileAlignment must not exceed SectionAlignment)
# INFERRED-FILEALIGN: error: /filealign:8192 exceeds /align:4096 for arm64 (PE FileAlignment must not exceed SectionAlignment)
# ARM64EC-FILEALIGN: error: /filealign:8192 exceeds /align:4096 for arm64ec (PE FileAlignment must not exceed SectionAlignment)
# ARM64X-FILEALIGN: error: /filealign:8192 exceeds /align:4096 for arm64x (PE FileAlignment must not exceed SectionAlignment)

# RUN: lld-link /machine:arm64 /entry:main /subsystem:console /align:4096 /out:%t.exe %t.arm64.obj
# RUN: llvm-readobj --file-headers %t.exe | FileCheck %s --check-prefix=HEADER
# HEADER: SectionAlignment: 4096

# The equal-alignment case is valid and must continue to link.
# RUN: lld-link /machine:arm64 /entry:main /subsystem:console /align:8192 /filealign:8192 /out:%t.exe %t.arm64.obj
# RUN: llvm-readobj --file-headers %t.exe | FileCheck %s --check-prefix=EQUAL-ALIGN
# EQUAL-ALIGN: SectionAlignment: 8192
# EQUAL-ALIGN: FileAlignment: 8192

# RUN: lld-link /entry:main /subsystem:console /out:%t.exe %t.arm64.obj

    .text
    .globl main
main:
    ret
