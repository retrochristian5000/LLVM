//===-- pragma-no-sanitize.cpp --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Clang handles no_sanitize("thread") pragmas during compilation. TSan's
// runtime suppression scopes do not interpret pragma text.
//
// RUN: %clangxx_tsan -O1 -S -emit-llvm %s -o - | FileCheck %s

#pragma clang attribute push (__attribute__((no_sanitize("thread"))), apply_to = function)
extern "C" __attribute__((noinline)) int pragma_uninstrumented(int *ptr) {
  return *ptr;
}
#pragma clang attribute pop

extern "C" __attribute__((noinline)) int pragma_instrumented(int *ptr) {
  return *ptr;
}

// CHECK-LABEL: define{{.*}}@pragma_uninstrumented(
// CHECK-NOT: call void @__tsan_read4
// CHECK: ret i32
// CHECK-LABEL: define{{.*}}@pragma_instrumented(
// CHECK: call void @__tsan_read4
