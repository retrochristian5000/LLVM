# REQUIRES: aarch64
#
## Check the actual ARM64e USERLAND24 pointer words, not just the presence
## of LC_DYLD_CHAINED_FIXUPS. All four PAC keys, 16-bit diversity values,
## address-diversity bits and the 8-byte chained next fields must survive.
## Cover both a local authenticated rebase and an imported authenticated bind.
#
# RUN: rm -rf %t; split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/producer.o %t/producer.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -install_name @rpath/libauth-producer.dylib \
# RUN:   %t/producer.o -o %t/libauth-producer.dylib
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/consumer.o %t/consumer.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/consumer.o %t/libauth-producer.dylib \
# RUN:   -o %t/consumer.dylib
# RUN: llvm-objdump --macho --chained-fixups %t/consumer.dylib | \
# RUN:   FileCheck %s --check-prefix=FIXUPS
# RUN: %python %S/Inputs/arm64e-auth-metadata.py %t/consumer.dylib
#
# FIXUPS: pointer_format = 12 (DYLD_CHAINED_PTR_ARM64E_USERLAND24)
# FIXUPS: _external
#
#--- producer.s
.text
.globl _external
_external:
  ret
#
#--- consumer.s
.text
_local:
  ret
.data
.p2align 3
## Rebase; four ARM PAC keys IA, IB, DA, DB.
.quad _local@AUTH(ia,42,addr)
.quad _local@AUTH(ib,513)
.quad _local@AUTH(da,7,addr)
.quad _local@AUTH(db,65535)
## Bind; the same four PAC keys must be preserved for imported functions.
.quad _external@AUTH(ia,42,addr)
.quad _external@AUTH(ib,513)
.quad _external@AUTH(da,7,addr)
.quad _external@AUTH(db,65535)
