//===-- tsan_shadow_test.cpp ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
//===----------------------------------------------------------------------===//
#include "tsan_platform.h"
#include "tsan_rtl.h"
#include "tsan_warn_runtime.h"
#include "gtest/gtest.h"

#if SANITIZER_APPLE && !SANITIZER_GO
#  include "tsan_array_darwin.h"
#endif

namespace __tsan {

struct Region {
  uptr start;
  uptr end;
};

void CheckShadow(const Shadow *s, Sid sid, Epoch epoch, uptr addr, uptr size,
                 AccessType typ) {
  uptr addr1 = 0;
  uptr size1 = 0;
  AccessType typ1 = 0;
  s->GetAccess(&addr1, &size1, &typ1);
  CHECK_EQ(s->sid(), sid);
  CHECK_EQ(s->epoch(), epoch);
  CHECK_EQ(addr1, addr);
  CHECK_EQ(size1, size);
  CHECK_EQ(typ1, typ);
}

TEST(Shadow, Shadow) {
  Sid sid = static_cast<Sid>(11);
  Epoch epoch = static_cast<Epoch>(22);
  FastState fs;
  fs.SetSid(sid);
  fs.SetEpoch(epoch);
  CHECK_EQ(fs.sid(), sid);
  CHECK_EQ(fs.epoch(), epoch);
  CHECK_EQ(fs.GetIgnoreBit(), false);
  fs.SetIgnoreBit();
  CHECK_EQ(fs.GetIgnoreBit(), true);
  fs.ClearIgnoreBit();
  CHECK_EQ(fs.GetIgnoreBit(), false);

  Shadow s0(fs, 1, 2, kAccessWrite);
  CheckShadow(&s0, sid, epoch, 1, 2, kAccessWrite);
  Shadow s1(fs, 2, 3, kAccessRead);
  CheckShadow(&s1, sid, epoch, 2, 3, kAccessRead);
  Shadow s2(fs, 0xfffff8 + 4, 1, kAccessWrite | kAccessAtomic);
  CheckShadow(&s2, sid, epoch, 4, 1, kAccessWrite | kAccessAtomic);
  Shadow s3(fs, 0xfffff8 + 0, 8, kAccessRead | kAccessAtomic);
  CheckShadow(&s3, sid, epoch, 0, 8, kAccessRead | kAccessAtomic);

  CHECK(!s0.IsBothReadsOrAtomic(kAccessRead | kAccessAtomic));
  CHECK(!s1.IsBothReadsOrAtomic(kAccessAtomic));
  CHECK(!s1.IsBothReadsOrAtomic(kAccessWrite));
  CHECK(s1.IsBothReadsOrAtomic(kAccessRead));
  CHECK(s2.IsBothReadsOrAtomic(kAccessAtomic));
  CHECK(!s2.IsBothReadsOrAtomic(kAccessWrite));
  CHECK(!s2.IsBothReadsOrAtomic(kAccessRead));
  CHECK(s3.IsBothReadsOrAtomic(kAccessAtomic));
  CHECK(!s3.IsBothReadsOrAtomic(kAccessWrite));
  CHECK(s3.IsBothReadsOrAtomic(kAccessRead));

  CHECK(!s0.IsRWWeakerOrEqual(kAccessRead | kAccessAtomic));
  CHECK(s1.IsRWWeakerOrEqual(kAccessWrite));
  CHECK(s1.IsRWWeakerOrEqual(kAccessRead));
  CHECK(!s1.IsRWWeakerOrEqual(kAccessWrite | kAccessAtomic));

  CHECK(!s2.IsRWWeakerOrEqual(kAccessRead | kAccessAtomic));
  CHECK(s2.IsRWWeakerOrEqual(kAccessWrite | kAccessAtomic));
  CHECK(s2.IsRWWeakerOrEqual(kAccessRead));
  CHECK(s2.IsRWWeakerOrEqual(kAccessWrite));

  CHECK(s3.IsRWWeakerOrEqual(kAccessRead | kAccessAtomic));
  CHECK(s3.IsRWWeakerOrEqual(kAccessWrite | kAccessAtomic));
  CHECK(s3.IsRWWeakerOrEqual(kAccessRead));
  CHECK(s3.IsRWWeakerOrEqual(kAccessWrite));

  Shadow sro(Shadow::kRodata);
  CheckShadow(&sro, static_cast<Sid>(0), kEpochZero, 0, 0, kAccessRead);
}

TEST(Shadow, Mapping) {
  static int global;
  int stack;
  void *heap = malloc(0);
  free(heap);

  CHECK(IsAppMem((uptr)&global));
  CHECK(IsAppMem((uptr)&stack));
  CHECK(IsAppMem((uptr)heap));

  CHECK(IsShadowMem(MemToShadow((uptr)&global)));
  CHECK(IsShadowMem(MemToShadow((uptr)&stack)));
  CHECK(IsShadowMem(MemToShadow((uptr)heap)));
}

TEST(Shadow, ApplicationMemoryRangeValidation) {
  const uptr windows[][2] = {
      {LoAppMemBeg(), LoAppMemEnd()},
      {MidAppMemBeg(), MidAppMemEnd()},
      {HeapMemBeg(), HeapMemEnd()},
      {HiAppMemBeg(), HiAppMemEnd()},
  };
  EXPECT_FALSE(IsValidAppMemoryRange(0, 0));
  EXPECT_FALSE(IsValidAppMemoryRange(~(uptr)0 - 1, 4));
  EXPECT_EQ(AppMemoryRangeError::kEmpty, DiagnoseAppMemoryRange(0, 0));
  EXPECT_EQ(AppMemoryRangeError::kOverflow,
            DiagnoseAppMemoryRange(~(uptr)0 - 1, 4));
  uptr first_unsupported = 1;
  EXPECT_EQ(AppMemoryRangeError::kOutsideAppMemory,
            DiagnoseAppMemoryRange(0, 1, &first_unsupported));
  EXPECT_EQ(0U, first_unsupported);
  EXPECT_STREQ("range crosses a gap between instrumentable memory windows",
               AppMemoryRangeErrorDescription(AppMemoryRangeError::kCrossesGap));

  for (const auto &window : windows) {
    if (window[0] == window[1])
      continue;
    EXPECT_TRUE(IsValidAppMemoryRange(window[0], window[1] - window[0]));
    EXPECT_TRUE(IsValidAppMemoryRange(window[1] - 1, 1));

    bool has_adjacent_window = false;
    for (const auto &other : windows)
      has_adjacent_window |= other[0] == window[1] && other[0] < other[1];
    EXPECT_EQ(has_adjacent_window,
              IsValidAppMemoryRange(window[1] - 1, 2));
    if (!has_adjacent_window) {
      first_unsupported = 0;
      EXPECT_EQ(AppMemoryRangeError::kCrossesGap,
                DiagnoseAppMemoryRange(window[1] - 1, 2,
                                       &first_unsupported));
      EXPECT_EQ(window[1], first_unsupported);
    }
  }
}

TEST(Shadow, PosixMunmapPageRange) {
  const uptr page = GetPageSizeCached();
  const uptr addr = RoundUpTo(LoAppMemBeg(), page);
  ASSERT_TRUE(IsValidAppMemoryRange(addr, 2 * page));
  uptr size = 0;
  EXPECT_TRUE(GetPosixMunmapRange(addr, 1, page, &size));
  EXPECT_EQ(page, size);
  EXPECT_TRUE(GetPosixMunmapRange(addr, page + 1, page, &size));
  EXPECT_EQ(2 * page, size);

  size = 123;
  EXPECT_FALSE(GetPosixMunmapRange(addr + 1, page, page, &size));
  EXPECT_FALSE(GetPosixMunmapRange(addr, 0, page, &size));
  EXPECT_FALSE(GetPosixMunmapRange(addr, ~(uptr)0, page, &size));
  EXPECT_FALSE(GetPosixMunmapRange(addr, ~(uptr)0 - addr, page, &size));
  EXPECT_EQ(123U, size);

  const uptr last_page = RoundDownTo(LoAppMemEnd(), page) - page;
  ASSERT_TRUE(IsValidAppMemoryRange(last_page, page));
  EXPECT_TRUE(GetPosixMunmapRange(last_page, page, page, &size));
  EXPECT_EQ(page, size);
  // Translate only the last application byte, not the exclusive endpoint.
  EXPECT_EQ(MemToShadow(last_page) + page / kShadowCell * kShadowCnt,
            MemToShadow(last_page + page - 1) + kShadowCnt);
  if (!IsValidAppMemoryRange(last_page, 2 * page)) {
    size = 123;
    EXPECT_FALSE(GetPosixMunmapRange(last_page, page + 1, page, &size));
    EXPECT_EQ(123U, size);
  }
}

TEST(Shadow, ShadowCellRangeValidation) {
  const uptr addr = RoundUpTo(LoAppMemBeg(), kShadowCell);
  ASSERT_TRUE(IsValidAppMemoryRange(addr, 2 * kShadowCell));
  uptr begin = 0;
  uptr end = 0;
  EXPECT_TRUE(GetShadowCellRange(addr + 1, kShadowCell, &begin, &end));
  EXPECT_EQ(addr, begin);
  EXPECT_EQ(addr + 2 * kShadowCell, end);

  // Empty or overflowing ranges must not change the output arguments.
  begin = 1;
  end = 2;
  EXPECT_FALSE(GetShadowCellRange(addr + 1, 0, &begin, &end));
  EXPECT_FALSE(GetShadowCellRange(addr, ~(uptr)0, &begin, &end));
  EXPECT_FALSE(GetShadowCellRange(addr, ~(uptr)0 - addr, &begin, &end));
  EXPECT_EQ(1U, begin);
  EXPECT_EQ(2U, end);

  // Even valid endpoints do not make an interval across a reserved gap safe.
  const uptr windows[][2] = {
      {LoAppMemBeg(), LoAppMemEnd()},
      {MidAppMemBeg(), MidAppMemEnd()},
      {HeapMemBeg(), HeapMemEnd()},
      {HiAppMemBeg(), HiAppMemEnd()},
  };
  for (const auto &left : windows) {
    if (left[0] == left[1])
      continue;
    for (const auto &right : windows) {
      if (right[0] == right[1] || right[0] <= left[1])
        continue;
      const uptr first = left[1] - kShadowCell;
      const uptr last = right[0];
      const uptr length = last - first + 1;
      if (DiagnoseAppMemoryRange(first, length) ==
          AppMemoryRangeError::kCrossesGap) {
        EXPECT_TRUE(IsAppMem(first));
        EXPECT_TRUE(IsAppMem(last));
        EXPECT_FALSE(GetShadowCellRange(first, length, &begin, &end));
        EXPECT_EQ(1U, begin);
        EXPECT_EQ(2U, end);
      }
    }
  }
}

#if SANITIZER_APPLE && !SANITIZER_GO
TEST(Shadow, DarwinTargetLayout) {
#if SANITIZER_IOS && !SANITIZER_IOSSIM
  EXPECT_EQ(static_cast<uptr>(MappingAppleAarch64::kShadowBeg), ShadowBeg());
  EXPECT_EQ(static_cast<uptr>(MappingAppleAarch64::kMetaShadowBeg), MetaShadowBeg());
  EXPECT_EQ(LoAppMemEnd(), HeapMemBeg());
  EXPECT_LT(HeapMemEnd(), ShadowBeg());
  EXPECT_LT(ShadowEnd(), MetaShadowBeg());
  EXPECT_LT(MetaShadowEnd(), HiAppMemBeg());
  EXPECT_EQ(HiAppMemBeg(), HiAppMemEnd());
#else
  EXPECT_EQ(static_cast<uptr>(Mapping48AddressSpace::kShadowBeg), ShadowBeg());
  EXPECT_EQ(static_cast<uptr>(Mapping48AddressSpace::kMetaShadowBeg), MetaShadowBeg());
  EXPECT_LT(LoAppMemEnd(), ShadowBeg());
  EXPECT_LT(MidAppMemBeg(), MidAppMemEnd());
  EXPECT_LT(HiAppMemBeg(), HiAppMemEnd());
#endif
}

TEST(Shadow, DarwinMachVmPageRounding) {
  const uptr page_size = GetPageSizeCached();
  const uptr heap_end = HeapMemEnd();
  ASSERT_GT(heap_end - HeapMemBeg(), page_size);
  EXPECT_TRUE(DarwinArrayRangeIsSupported(heap_end - page_size, page_size));
  // The final byte in the heap plus one byte outside it spans two pages.
  EXPECT_FALSE(DarwinArrayRangeIsSupported(heap_end - 1, 2));
  const DarwinArrayRangeDiagnosis diagnosis =
      DiagnoseDarwinArrayRange(heap_end - 1, 2);
  EXPECT_EQ(AppMemoryRangeError::kCrossesGap, diagnosis.error);
  EXPECT_EQ(heap_end - page_size, diagnosis.page_begin);
  EXPECT_EQ(heap_end + page_size, diagnosis.page_end);
  EXPECT_EQ(heap_end, diagnosis.first_unsupported);
}
#endif

TEST(Shadow, Celling) {
  u64 aligned_data[4];
  char *data = (char*)aligned_data;
  CHECK(IsAligned(reinterpret_cast<uptr>(data), kShadowSize));
  RawShadow *s0 = MemToShadow((uptr)&data[0]);
  CHECK(IsAligned(reinterpret_cast<uptr>(s0), kShadowSize));
  for (unsigned i = 1; i < kShadowCell; i++)
    CHECK_EQ(s0, MemToShadow((uptr)&data[i]));
  for (unsigned i = kShadowCell; i < 2*kShadowCell; i++)
    CHECK_EQ(s0 + kShadowCnt, MemToShadow((uptr)&data[i]));
  for (unsigned i = 2*kShadowCell; i < 3*kShadowCell; i++)
    CHECK_EQ(s0 + 2 * kShadowCnt, MemToShadow((uptr)&data[i]));
}

// Detect is the Mapping has kBroken field.
template <uptr>
struct Has {
  typedef bool Result;
};

template <typename Mapping>
bool broken(...) {
  return false;
}

template <typename Mapping>
bool broken(uptr what, typename Has<Mapping::kBroken>::Result = false) {
  return Mapping::kBroken & what;
}

static int CompareRegion(const void *region_a, const void *region_b) {
  uptr start_a = ((const struct Region *)region_a)->start;
  uptr start_b = ((const struct Region *)region_b)->start;

  if (start_a < start_b) {
    return -1;
  } else if (start_a > start_b) {
    return 1;
  } else {
    return 0;
  }
}

template <typename Mapping>
static void AddMetaRegion(struct Region *shadows, int *num_regions, uptr start,
                          uptr end) {
  // If the app region is not empty, add its meta to the array.
  if (start != end) {
    shadows[*num_regions].start = (uptr)MemToMetaImpl::Apply<Mapping>(start);
    shadows[*num_regions].end = (uptr)MemToMetaImpl::Apply<Mapping>(end - 1);
    *num_regions = (*num_regions) + 1;
  }
}

struct MappingTest {
  template <typename Mapping>
  static void Apply() {
    // Easy (but ugly) way to print the mapping name.
    Printf("%s\n", __PRETTY_FUNCTION__);
    TestRegion<Mapping>(Mapping::kLoAppMemBeg, Mapping::kLoAppMemEnd);
    TestRegion<Mapping>(Mapping::kMidAppMemBeg, Mapping::kMidAppMemEnd);
    TestRegion<Mapping>(Mapping::kHiAppMemBeg, Mapping::kHiAppMemEnd);
    TestRegion<Mapping>(Mapping::kHeapMemBeg, Mapping::kHeapMemEnd);

    TestDisjointMetas<Mapping>();

    // Not tested: the ordering of regions (low app vs. shadow vs. mid app
    // etc.). That is enforced at runtime by CheckAndProtect.
  }

  template <typename Mapping>
  static void TestRegion(uptr beg, uptr end) {
    if (beg == end)
      return;
    Printf("checking region [0x%zx-0x%zx)\n", beg, end);
    uptr prev = 0;
    for (uptr p0 = beg; p0 <= end; p0 += (end - beg) / 256) {
      for (int x = -(int)kShadowCell; x <= (int)kShadowCell; x += kShadowCell) {
        const uptr p = RoundDown(p0 + x, kShadowCell);
        if (p < beg || p >= end)
          continue;
        const uptr s = MemToShadowImpl::Apply<Mapping>(p);
        u32 *const m = MemToMetaImpl::Apply<Mapping>(p);
        const uptr r = ShadowToMemImpl::Apply<Mapping>(s);
        Printf("  addr=0x%zx: shadow=0x%zx meta=%p reverse=0x%zx\n", p, s, m,
               r);
        CHECK(IsAppMemImpl::Apply<Mapping>(p));
        if (!broken<Mapping>(kBrokenMapping))
          CHECK(IsShadowMemImpl::Apply<Mapping>(s));
        CHECK(IsMetaMemImpl::Apply<Mapping>(reinterpret_cast<uptr>(m)));
        CHECK_EQ(p, RestoreAddrImpl::Apply<Mapping>(CompressAddr(p)));
        if (!broken<Mapping>(kBrokenReverseMapping))
          CHECK_EQ(p, r);
        if (prev && !broken<Mapping>(kBrokenLinearity)) {
          // Ensure that shadow and meta mappings are linear within a single
          // user range. Lots of code that processes memory ranges assumes it.
          const uptr prev_s = MemToShadowImpl::Apply<Mapping>(prev);
          u32 *const prev_m = MemToMetaImpl::Apply<Mapping>(prev);
          CHECK_EQ(s - prev_s, (p - prev) * kShadowMultiplier);
          CHECK_EQ(m - prev_m, (p - prev) / kMetaShadowCell);
        }
        prev = p;
      }
    }
  }

  template <typename Mapping>
  static void TestDisjointMetas() {
    // Checks that the meta for each app region does not overlap with
    // the meta for other app regions. For example, the meta for a high
    // app pointer shouldn't be aliased to the meta of a mid app pointer.
    // Notice that this is important even though there does not exist a
    // MetaToMem function.
    // (If a MetaToMem function did exist, we could simply
    // check in the TestRegion function that it inverts MemToMeta.)
    //
    // We don't try to be clever by allowing the non-PIE (low app)
    // and PIE (mid and high app) meta regions to overlap.
    struct Region metas[4];
    int num_regions = 0;
    AddMetaRegion<Mapping>(metas, &num_regions, Mapping::kLoAppMemBeg,
                           Mapping::kLoAppMemEnd);
    AddMetaRegion<Mapping>(metas, &num_regions, Mapping::kMidAppMemBeg,
                           Mapping::kMidAppMemEnd);
    AddMetaRegion<Mapping>(metas, &num_regions, Mapping::kHiAppMemBeg,
                           Mapping::kHiAppMemEnd);
    AddMetaRegion<Mapping>(metas, &num_regions, Mapping::kHeapMemBeg,
                           Mapping::kHeapMemEnd);

    // It is not required that the low app shadow is below the mid app
    // shadow etc., hence we sort the shadows.
    qsort(metas, num_regions, sizeof(struct Region), CompareRegion);

    for (int i = 0; i < num_regions; i++)
      Printf("[0x%lu, 0x%lu]\n", metas[i].start, metas[i].end);

    if (!broken<Mapping>(kBrokenAliasedMetas))
      for (int i = 1; i < num_regions; i++)
        CHECK(metas[i - 1].end <= metas[i].start);
  }
};

TEST(Shadow, AllMappings) { ForEachMapping<MappingTest>(); }

}  // namespace __tsan
