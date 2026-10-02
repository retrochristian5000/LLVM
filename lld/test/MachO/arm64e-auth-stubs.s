# REQUIRES: aarch64

## arm64e chained-fixup stubs must authenticate imported function pointers.
## Keep the authenticated pointer in __auth_got and use the slot address as
## BRAA's discriminator.

# RUN: rm -rf %t; split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/foo.o %t/foo.s
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/test.o %t/test.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -install_name @rpath/libfoo.dylib %t/foo.o -o %t/libfoo.dylib
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains %t/libfoo.dylib %t/test.o -o %t/libtest.dylib
# RUN: llvm-otool -l %t/libtest.dylib | FileCheck %s --check-prefix=SECTIONS
# RUN: llvm-objdump --no-print-imm-hex --macho -d --no-show-raw-insn \
# RUN:   --section="__TEXT,__auth_stubs" %t/libtest.dylib | \
# RUN:   FileCheck %s --check-prefix=STUB
# RUN: llvm-objdump --macho --chained-fixups %t/libtest.dylib | \
# RUN:   FileCheck %s --check-prefix=FIXUPS

# SECTIONS: sectname __auth_stubs
# SECTIONS: sectname __auth_got

# STUB-LABEL: Contents of (__TEXT,__auth_stubs) section
# STUB:       adrp x17
# STUB-NEXT:  add x17, x17
# STUB-NEXT:  ldr x16, [x17]
# STUB-NEXT:  braa x16, x17

# FIXUPS: pointer_format = 12 (DYLD_CHAINED_PTR_ARM64E_USERLAND24)
# FIXUPS: _foo

#--- foo.s
.text
.globl _foo
_foo:
  ret

#--- test.s
.text
.globl _caller
_caller:
  bl _foo
  ret
