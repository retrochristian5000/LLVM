//===-- malloc-zone-hooks.cpp --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Verify that Apple malloc-zone allocations invoke registered sanitizer hooks.
// The Darwin signal-unsafe allocation reporter is tested separately.
//
// RUN: %clangxx_tsan -O1 %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'

#include <malloc/malloc.h>
#include <sanitizer/allocator_interface.h>
#include <stdio.h>

static const volatile void *allocated_ptr;
static int allocation_events;
static int free_events;

__attribute__((no_sanitize("thread")))
static void OnMalloc(const volatile void *ptr, size_t size) {
  if (size == 157 && ptr != nullptr) {
    allocated_ptr = ptr;
    ++allocation_events;
  }
}

__attribute__((no_sanitize("thread")))
static void OnFree(const volatile void *ptr) {
  if (ptr != nullptr && ptr == allocated_ptr)
    ++free_events;
}

int main() {
  if (!__sanitizer_install_malloc_and_free_hooks(OnMalloc, OnFree))
    return 1;

  malloc_zone_t *zone = malloc_default_zone();
  if (!zone)
    return 2;

  void *ptr = malloc_zone_malloc(zone, 157);
  if (!ptr)
    return 3;

  const bool allocated = allocated_ptr == ptr && allocation_events > 0;
  malloc_zone_free(zone, ptr);
  if (!allocated || free_events == 0)
    return 4;

  puts("Apple malloc-zone hooks: OK");
  return 0;
}

// CHECK: Apple malloc-zone hooks: OK
