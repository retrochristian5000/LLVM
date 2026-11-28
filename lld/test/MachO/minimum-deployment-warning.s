# REQUIRES: x86
# RUN: split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-macosx27.2 %t/compatible.s -o %t/compatible.o
# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-macosx27.3 %t/newer.s -o %t/newer.o

## A newer SDK alone does not make an input incompatible.
# RUN: llvm-objdump --macho --private-headers %t/compatible.o | FileCheck %s --check-prefix=METADATA
# METADATA: cmd LC_BUILD_VERSION
# METADATA: sdk 27.3
# METADATA: minos 27.2
# RUN: %lld -platform_version macos 27.2 27.3 -o %t/compatible %t/compatible.o

## A genuinely newer input deployment target still triggers the warning.
# RUN: not %lld -platform_version macos 27.2 27.3 -o %t/newer %t/newer.o 2>&1 | FileCheck %s --check-prefix=NEWER
# NEWER: has version 27.3{{(\.0)?}}, which is newer than target minimum of 27.2{{(\.0)?}}
# NEWER-SAME: the input minimum OS version, not its SDK version, is newer.
# NEWER-SAME: Rebuild the input for the target minimum OS or raise the deployment target.

## Equal minimum versions remain compatible.
# RUN: %lld -platform_version macos 27.3 27.3 -o %t/equal %t/newer.o

#--- compatible.s
.build_version macos, 27, 2 sdk_version 27, 3
.text
.globl _main
_main:
  xor %eax, %eax
  ret

#--- newer.s
.build_version macos, 27, 3 sdk_version 27, 3
.text
.globl _main
_main:
  xor %eax, %eax
  ret
