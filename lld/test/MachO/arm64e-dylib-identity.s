# REQUIRES: aarch64
#
## Exercise the exact LC_ID_DYLIB -> LC_LOAD_DYLIB identity/version round trip
## on versioned user ABI 0 arm64e. The consumer must retain the producer's
## install name, not its local input filename or the consumer's own ID.
# RUN: rm -rf %t; split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/provider.o %t/provider.s
# RUN: llvm-mc -filetype=obj -triple=arm64e-apple-macos -o %t/client.o %t/client.s
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib \
# RUN:   -fixup_chains -install_name @rpath/libwhp-identity.dylib \
# RUN:   -current_version 12.34.56 -compatibility_version 3.4.5 \
# RUN:   %t/provider.o -o %t/provider.dylib
# RUN: %python %S/Inputs/arm64e-dylib-identity.py %t/provider.dylib \
# RUN:   6 id @rpath/libwhp-identity.dylib 12.34.56 3.4.5
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib \
# RUN:   -fixup_chains -install_name @rpath/libwhp-consumer.dylib \
# RUN:   -current_version 7.8.9 -compatibility_version 1.2.3 \
# RUN:   %t/client.o %t/provider.dylib -o %t/client.dylib
# RUN: %python %S/Inputs/arm64e-dylib-identity.py %t/client.dylib \
# RUN:   6 id @rpath/libwhp-consumer.dylib 7.8.9 1.2.3
# RUN: %python %S/Inputs/arm64e-dylib-identity.py %t/client.dylib \
# RUN:   6 load @rpath/libwhp-identity.dylib 12.34.56 3.4.5
#
## An unspecified install name uses the final output (not a temp per-arch
## output), and unspecified versions are both zero, matching ld64.
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib \
# RUN:   %t/provider.o -o %t/default.dylib
# RUN: %python %S/Inputs/arm64e-dylib-identity.py %t/default.dylib \
# RUN:   6 id %t/default.dylib 0 0
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib \
# RUN:   -final_output @rpath/libwhp-universal.dylib \
# RUN:   %t/provider.o -o %t/temporary-slice.dylib
# RUN: %python %S/Inputs/arm64e-dylib-identity.py %t/temporary-slice.dylib \
# RUN:   6 id @rpath/libwhp-universal.dylib 0 0
#
## Exercise ld64-compatible aliases and precedence over -final_output.
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -dylib \
# RUN:   -final_output @rpath/wrong-default.dylib \
# RUN:   -dylib_install_name @loader_path/libwhp-alias.dylib \
# RUN:   -dylib_current_version 65535.255.255 \
# RUN:   -dylib_compatibility_version 1.0.0 \
# RUN:   %t/provider.o -o %t/alias.dylib
# RUN: %python %S/Inputs/arm64e-dylib-identity.py %t/alias.dylib \
# RUN:   6 id @loader_path/libwhp-alias.dylib 65535.255.255 1.0.0
#
## Plugins are MH_BUNDLE, even when their filename ends in .dylib. They must
## not get LC_ID_DYLIB. Version options are invalid outside -dylib output.
# RUN: %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 -bundle \
# RUN:   -install_name @rpath/should-not-appear.dylib \
# RUN:   %t/provider.o -o %t/plugin.dylib
# RUN: %python %S/Inputs/arm64e-dylib-identity.py %t/plugin.dylib 8 none
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -bundle -current_version 2.3 -compatibility_version 1.0 \
# RUN:   %t/provider.o -o %t/bad.dylib 2>&1 | FileCheck %s --check-prefix=BAD-TYPE
# BAD-TYPE-DAG: -current_version 2.3: only valid with -dylib
# BAD-TYPE-DAG: -compatibility_version 1.0: only valid with -dylib
#
## The default version zero is valid; writing zero explicitly is not.
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -current_version 0.1 -compatibility_version 0 \
# RUN:   %t/provider.o -o %t/bad-zero.dylib 2>&1 | FileCheck %s --check-prefix=BAD-ZERO
# BAD-ZERO-DAG: -current_version 0.1: malformed version
# BAD-ZERO-DAG: -compatibility_version 0: malformed version
#
# RUN: not %no-arg-lld -arch arm64e -platform_version macos 13.0 13.0 \
# RUN:   -dylib -current_version 2..3 -compatibility_version 1.2. \
# RUN:   %t/provider.o -o %t/bad-dots.dylib 2>&1 | FileCheck %s --check-prefix=BAD-DOTS
# BAD-DOTS-DAG: -current_version 2..3: malformed version
# BAD-DOTS-DAG: -compatibility_version 1.2.: malformed version
#
#--- provider.s
.text
.globl _whp_version_provider
_whp_version_provider:
  ret
#
#--- client.s
.text
.globl _whp_version_caller
_whp_version_caller:
  bl _whp_version_provider
  ret
.data
.p2align 3
.quad _whp_version_provider@AUTH(ia,42,addr)
