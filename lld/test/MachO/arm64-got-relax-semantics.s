# REQUIRES: aarch64
# RUN: llvm-mc -filetype=obj -triple=arm64-apple-macosx11.0 %s -o %t.o
# RUN: %no-lsystem-lld -arch arm64 -dylib -no_fixup_chains %t.o -o %t.dylib
# RUN: llvm-objdump --macho -d %t.dylib | FileCheck %s

# CHECK-LABEL: _discard:
# CHECK: adrp x8,
# CHECK-NEXT: ldr xzr, [x8
# CHECK-NEXT: ret
# CHECK-LABEL: _same_symbol:
# CHECK: adrp x0,
# CHECK-NEXT: ldr x0, [x0
# CHECK-NEXT: ret
# CHECK-LABEL: _narrow:
# CHECK: adrp x0,
# CHECK-NEXT: ldr w0, [x0
# CHECK-NEXT: ret
# CHECK-LABEL: _direct:
# CHECK: adrp x0,
# CHECK-NEXT: {{add|mov}} x0, x0
# CHECK-NEXT: ret

.text
.p2align 2
.globl _discard, _same_symbol, _narrow, _direct
_discard:
  adrp x8, _slot@GOTPAGE
  ldr xzr, [x8, _slot@GOTPAGEOFF]
  ret
_same_symbol:
  adrp x0, _slot@GOTPAGE
  ldr x0, [x0, _slot@GOTPAGEOFF]
  ret
_narrow:
  adrp x0, _narrow_slot@GOTPAGE
  ldr w0, [x0, _narrow_slot@GOTPAGEOFF]
  ret
_direct:
  adrp x0, _direct_slot@GOTPAGE
  ldr x0, [x0, _direct_slot@GOTPAGEOFF]
  ret
.data
.p2align 3
_slot:
  .quad 0
_narrow_slot:
  .quad 0
_direct_slot:
  .quad 0
.subsections_via_symbols
