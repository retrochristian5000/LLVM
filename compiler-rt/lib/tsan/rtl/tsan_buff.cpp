//===-- tsan_buff.cpp -----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Allocation, initialization, and in-place reversal of stack-trace PC buffers.
// No shadow-stack or trace-event ownership is changed here.

#include "tsan_buff.h"

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_libc.h"
#include "tsan_mman.h"
#include "tsan_stack_trace.h"

namespace __tsan {

void ReversePcBuffer(uptr *pcs, uptr count) {
  if (!count)
    return;
  CHECK(pcs);
  for (uptr i = 0; i < count / 2; ++i)
    Swap(pcs[i], pcs[count - i - 1]);
}

VarSizeStackTrace::VarSizeStackTrace()
    : StackTrace(nullptr, 0), trace_buffer(nullptr) {}

VarSizeStackTrace::~VarSizeStackTrace() {
  ResizeBuffer(0);
}

void VarSizeStackTrace::ResizeBuffer(uptr new_size) {
  // Prevent wrapped byte counts before releasing the existing allocation.
  CHECK_LE(new_size, ~(uptr)0 / sizeof(*trace_buffer));
  Free(trace_buffer);
  trace_buffer = new_size
                     ? static_cast<uptr *>(Alloc(new_size * sizeof(*trace_buffer)))
                     : nullptr;
  trace = trace_buffer;
  size = new_size;
}

void VarSizeStackTrace::Init(const uptr *pcs, uptr cnt, uptr extra_top_pc) {
  const uptr extra = !!extra_top_pc;
  CHECK_LE(cnt, ~(uptr)0 - extra);
  if (cnt)
    CHECK(pcs);
  ResizeBuffer(cnt + extra);
  // Do not call memcpy with null pointers for an empty trace.
  if (cnt)
    internal_memcpy(trace_buffer, pcs, cnt * sizeof(*trace_buffer));
  if (extra)
    trace_buffer[cnt] = extra_top_pc;
}

void VarSizeStackTrace::ReverseOrder() {
  ReversePcBuffer(trace_buffer, size);
}

}  // namespace __tsan
