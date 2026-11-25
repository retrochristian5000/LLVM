; REQUIRES: x86
;
;; Changing CS PGO mode, the instrumentation output name, the profile contents
;; in place, or the mismatch diagnostic policy must not reuse stale objects.
; RUN: rm -rf %t; split-file %s %t
; RUN: opt -module-hash -module-summary %t/input.ll -o %t/input.o
; RUN: mkdir %t/cache
;
; RUN: %lld -dylib -cache_path_lto %t/cache %t/input.o -o %t/out
; RUN: ls %t/cache/llvmcache-* | count 1
; RUN: %lld -dylib -cache_path_lto %t/cache --cs-profile-generate --cs-profile-path=%t/profile.profdata %t/input.o -o %t/out
; RUN: ls %t/cache/llvmcache-* | count 2
; RUN: %lld -dylib -cache_path_lto %t/cache --cs-profile-generate --cs-profile-path=%t/other.profraw %t/input.o -o %t/out
; RUN: ls %t/cache/llvmcache-* | count 3
; RUN: %lld -dylib -cache_path_lto %t/cache --cs-profile-generate --cs-profile-path=%t/profile.profdata %t/input.o -o %t/out
; RUN: ls %t/cache/llvmcache-* | count 3
;
;; Profile generation and profile use have different cache identities even
;; when the same profile path is passed to the linker.
; RUN: llvm-profdata merge %t/first.proftext -o %t/profile.profdata
; RUN: %lld -dylib -cache_path_lto %t/cache --cs-profile-path=%t/profile.profdata --no-pgo-warn-mismatch %t/input.o -o %t/out
; RUN: ls %t/cache/llvmcache-* | count 4
; RUN: %lld -dylib -cache_path_lto %t/cache --cs-profile-path=%t/profile.profdata --no-pgo-warn-mismatch %t/input.o -o %t/out
; RUN: ls %t/cache/llvmcache-* | count 4
;
;; Replacing profile contents at the same path must trigger optimization.
; RUN: llvm-profdata merge %t/second.proftext -o %t/profile.profdata
; RUN: %lld -dylib -cache_path_lto %t/cache --cs-profile-path=%t/profile.profdata --no-pgo-warn-mismatch %t/input.o -o %t/out
; RUN: ls %t/cache/llvmcache-* | count 5
;
;; A permissive cached compilation must not bypass a strict mismatch check.
; RUN: not %lld -dylib -cache_path_lto %t/cache --cs-profile-path=%t/profile.profdata --pgo-warn-mismatch %t/input.o -o %t/out 2>&1 | FileCheck %s
; CHECK: function control flow change detected (hash mismatch) foo
;
;--- input.ll
target triple = "x86_64-apple-darwin"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"

@__llvm_profile_runtime = global i32 0, align 4

define i32 @foo() {
entry:
  ret i32 0
}

;--- first.proftext
:csir
foo
# Func Hash:
2277602155505015273
# Num Counters:
2
# Counter Values:
1
0

;--- second.proftext
:csir
foo
# Func Hash:
2277602155505015273
# Num Counters:
2
# Counter Values:
10
0
