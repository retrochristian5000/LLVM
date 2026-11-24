//===-- tsan_array.cpp ----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Bulk shadow-memory updates for array and other contiguous memory ranges.
// Individual access checks and race detection remain in tsan_rtl_access.cpp.
//
//===----------------------------------------------------------------------===//

#include "../rtl/tsan_rtl.h"

namespace __tsan {

#if defined(__aarch64__) && !SANITIZER_GO
// Native AArch64 implementation writes {value, 0, 0, 0} per shadow cell.
// The C++ entry point retains shadow-memory and alignment validation.
extern "C" void __tsan_shadow_set_aarch64(RawShadow *begin,
                                           RawShadow *end, u32 value);
#endif

void ShadowSet(RawShadow* p, RawShadow* end, RawShadow v) {
  DCHECK_LT(p, end);
  DCHECK(IsShadowMem(p));
  DCHECK(IsShadowMem(end - 1));
  UNUSED const uptr kAlign = kShadowCnt * kShadowSize;
  DCHECK_EQ(reinterpret_cast<uptr>(p) % kAlign, 0);
  DCHECK_EQ(reinterpret_cast<uptr>(end) % kAlign, 0);
#if defined(__aarch64__) && !SANITIZER_GO
  __tsan_shadow_set_aarch64(p, end, static_cast<u32>(v));
#elif !TSAN_VECTORIZE
  for (; p < end; p += kShadowCnt) {
    p[0] = v;
    for (uptr i = 1; i < kShadowCnt; i++) p[i] = Shadow::kEmpty;
  }
#else
  m128 vv = _mm_setr_epi32(
      static_cast<u32>(v), static_cast<u32>(Shadow::kEmpty),
      static_cast<u32>(Shadow::kEmpty), static_cast<u32>(Shadow::kEmpty));
  m128* vp = reinterpret_cast<m128*>(p);
  m128* vend = reinterpret_cast<m128*>(end);
  for (; vp < vend; vp++) _mm_store_si128(vp, vv);
#endif
}

static void MemoryRangeSet(uptr addr, uptr size, RawShadow val) {
  if (size == 0)
    return;
  DCHECK_EQ(addr % kShadowCell, 0);
  DCHECK_EQ(size % kShadowCell, 0);
  // Both callers validate the entire aligned interval before translating it.
  DCHECK(IsValidAppMemoryRange(addr, size));
  RawShadow* begin = MemToShadow(addr);
  RawShadow* end = begin + size / kShadowCell * kShadowCnt;
  // Only an empty shadow fill can be replaced with a zero-filled mapping.
  // An imitated write must preserve its epoch/access record in every cell.
  // This can commit physical pages, so keep the lazy remap for large resets.
  // UnmapOrDie/MmapFixedNoReserve does not work on Windows.
  if (SANITIZER_WINDOWS || val != Shadow::kEmpty ||
      size <= common_flags()->clear_shadow_mmap_threshold) {
    ShadowSet(begin, end, val);
    return;
  }
  // The region is big, reset only beginning and end.
  const uptr kPageSize = GetPageSizeCached();
  // Set at least first kPageSize/2 to page boundary.
  RawShadow* mid1 =
      Min(end, reinterpret_cast<RawShadow*>(RoundUp(
                   reinterpret_cast<uptr>(begin) + kPageSize / 2, kPageSize)));
  // begin must < mid1
  ShadowSet(begin, mid1, val);
  // Reset middle part.
  RawShadow* mid2 = RoundDown(end, kPageSize);
  if (mid2 > mid1) {
    if (!MmapFixedSuperNoReserve((uptr)mid1, (uptr)mid2 - (uptr)mid1))
      Die();
  }
  // Set the ending.
  if (mid2 < end)
    ShadowSet(mid2, end, val);
  else
    DCHECK_EQ(mid2, end);
}

void MemoryResetRange(ThreadState* thr, uptr pc, uptr addr, uptr size) {
  uptr begin, end;
  if (!GetShadowCellRange(addr, size, &begin, &end))
    return;
  MemoryRangeSet(begin, end - begin, Shadow::kEmpty);
}

void MemoryRangeImitateWrite(ThreadState* thr, uptr pc, uptr addr, uptr size) {
  DCHECK_EQ(addr % kShadowCell, 0);
  uptr begin, end;
  if (!GetShadowCellRange(addr, size, &begin, &end))
    return;
  const uptr aligned_size = end - begin;
  TraceMemoryAccessRange(thr, pc, begin, aligned_size, kAccessWrite);
  Shadow cur(thr->fast_state, 0, kShadowCell, kAccessWrite);
  MemoryRangeSet(begin, aligned_size, cur.raw());
}

void MemoryRangeImitateWriteOrResetRange(ThreadState* thr, uptr pc, uptr addr,
                                         uptr size) {
  if (thr->ignore_reads_and_writes == 0)
    MemoryRangeImitateWrite(thr, pc, addr, size);
  else
    MemoryResetRange(thr, pc, addr, size);
}

}  // namespace __tsan
