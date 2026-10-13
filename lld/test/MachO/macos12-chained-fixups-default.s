# REQUIRES: aarch64
#
# Apple's ld64 supports chained fixups by default from macOS 12.0. In
# particular, do not silently emit legacy bind/rebase load commands just
# because the deployment target is macOS 12 rather than 13.
# This ordinary arm64 test leaves the separate arm64e PAC policy untouched.
#
# RUN: llvm-mc -filetype=obj -triple=arm64-apple-macos -o %t.o %s
#
# macOS 11 is the legacy control.
# RUN: %no-arg-lld -arch arm64 -platform_version macos 11.0 13.0 \
# RUN:   -dylib %t.o -o %t-11.dylib
# RUN: llvm-objdump --macho --all-headers %t-11.dylib | \
# RUN:   FileCheck %s --check-prefix=LEGACY
# LEGACY: cmd LC_DYLD_INFO_ONLY
# LEGACY-NOT: cmd LC_DYLD_CHAINED_FIXUPS
#
# macOS 12 must enable chained fixups by default.
# RUN: %no-arg-lld -arch arm64 -platform_version macos 12.0 13.0 \
# RUN:   -dylib %t.o -o %t-12.dylib
# RUN: llvm-objdump --macho --all-headers %t-12.dylib | \
# RUN:   FileCheck %s --check-prefix=CHAINED
# CHAINED: cmd LC_DYLD_CHAINED_FIXUPS
# CHAINED-NOT: cmd LC_DYLD_INFO_ONLY
#
# Explicit opt-out on arm64 must still take precedence.
# RUN: %no-arg-lld -arch arm64 -platform_version macos 12.0 13.0 \
# RUN:   -dylib -no_fixup_chains %t.o -o %t-12-optout.dylib
# RUN: llvm-objdump --macho --all-headers %t-12-optout.dylib | \
# RUN:   FileCheck %s --check-prefix=LEGACY
#
# Explicit opt-in on an earlier macOS target remains possible.
# RUN: %no-arg-lld -arch arm64 -platform_version macos 11.0 13.0 \
# RUN:   -dylib -fixup_chains %t.o -o %t-11-optin.dylib
# RUN: llvm-objdump --macho --all-headers %t-11-optin.dylib | \
# RUN:   FileCheck %s --check-prefix=CHAINED

.text
.globl _whp_foo
_whp_foo:
  ret
.data
.p2align 3
.quad _whp_foo
