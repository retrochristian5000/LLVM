# REQUIRES: aarch64
#
## Malformed ARM64e relocation pairs must fail with a linker diagnostic, not
## read beyond the relocation table or confuse authenticated pointers with
## ordinary UNSIGNED relocations (both carry the UNSIGNED relocation attribute).
#
# RUN: rm -rf %t; mkdir -p %t
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos %s -o %t/good.o
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib %t/good.o -o %t/good.dylib
# RUN: %python %S/Inputs/arm64e-reloc-pairs.py %t/good.o %t
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib %t/missing-addend.o -o %t/bad1.dylib 2>&1 | FileCheck %s --check-prefix=MISSING-ADDEND
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib %t/auth-after-addend.o -o %t/bad2.dylib 2>&1 | FileCheck %s --check-prefix=AUTH-ADDEND
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib %t/missing-subtractor.o -o %t/bad3.dylib 2>&1 | FileCheck %s --check-prefix=MISSING-SUB
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib %t/auth-after-subtractor.o -o %t/bad4.dylib 2>&1 | FileCheck %s --check-prefix=AUTH-SUB
#
# MISSING-ADDEND: ARM64_RELOC_ADDEND missing its paired relocation
# AUTH-ADDEND: invalid ARM64_RELOC_ADDEND relocation pair
# MISSING-SUB: SUBTRACTOR missing its paired relocation
# AUTH-SUB: invalid SUBTRACTOR relocation pair

.text
.globl _target
_target:
  ret
.data
.p2align 3
.quad _target@AUTH(ia,1,addr)
.quad _target@AUTH(ib,2)
