# REQUIRES: aarch64

## arm64e pointer authentication needs dyld chained fixups. A legacy dyld
## binding path would generate unauthenticated stubs and cannot represent
## ARM64_RELOC_AUTHENTICATED_POINTER. Preserve arm64's existing opt-out.

# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t-arm64e.o %s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib %t-arm64e.o -o %t-arm64e.dylib
# RUN: llvm-objdump --macho --chained-fixups %t-arm64e.dylib | \
# RUN:   FileCheck %s --check-prefix=CHAINED
# CHAINED: chained fixups header (LC_DYLD_CHAINED_FIXUPS)
# CHAINED: pointer_format = 12 (DYLD_CHAINED_PTR_ARM64E_USERLAND24)

# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -no_fixup_chains %t-arm64e.o -o %t-disabled.dylib 2>&1 | \
# RUN:   FileCheck %s --check-prefix=DISABLED
# DISABLED: -no_fixup_chains is incompatible with arm64e

## The last explicit fixup option controls the outcome.
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -no_fixup_chains -fixup_chains %t-arm64e.o -o %t-last.dylib

# RUN: llvm-mc -filetype=obj -triple=arm64-apple-macos -o %t-arm64.o %s
# RUN: %no-arg-lld -arch arm64 -platform_version macos 13.0 13.0 \
# RUN:   -dylib -no_fixup_chains %t-arm64.o -o %t-arm64.dylib

.text
.globl _foo
_foo:
  ret

.data
.p2align 3
.quad _foo
