# REQUIRES: aarch64

## Fast Objective-C stubs on arm64e must authenticate _objc_msgSend through
## __auth_got using the slot address as BRAA's discriminator.

# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos %s -o %t.o
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -fixup_chains -objc_stubs_fast %t.o -o %t.dylib
# RUN: llvm-otool -vs __TEXT __objc_stubs %t.dylib | \
# RUN:   FileCheck %s --check-prefix=STUB
# RUN: llvm-otool -l %t.dylib | FileCheck %s --check-prefix=SECTIONS

# STUB:      _objc_msgSend$foo:
# STUB-NEXT: adrp    x1
# STUB-NEXT: ldr     x1
# STUB-NEXT: adrp    x17
# STUB-NEXT: add     x17, x17
# STUB-NEXT: ldr     x16, [x17]
# STUB-NEXT: braa    x16, x17
# STUB-NEXT: brk     #0x1
# STUB-NEXT: brk     #0x1

# SECTIONS: sectname __objc_stubs
# SECTIONS: sectname __auth_got

.section __TEXT,__objc_methname,cstring_literals
lselref:
  .asciz "foo"

.section __DATA,__objc_selrefs,literal_pointers,no_dead_strip
.p2align 3
.quad lselref

.text
.globl _objc_msgSend
_objc_msgSend:
  ret

.globl _caller
_caller:
  bl _objc_msgSend$foo
  ret
