//===-- tsan_objc_aslr.cpp -------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"

#if SANITIZER_APPLE && !SANITIZER_GO
#  include <dlfcn.h>
#  include <objc/runtime.h>

#  include "sanitizer_common/sanitizer_common.h"
#  include "sanitizer_common/sanitizer_ptrauth.h"
#  include "../rtl/tsan_platform.h"
#  include "tsan_objc_aslr.h"
#  if defined(TSAN_HAS_OBJC)
#    include "tsan_objc_runtime.h"
#  endif

namespace __tsan {

void ReportObjCImagePlacement() {
  // A non-intercepted Objective-C runtime symbol is a loader anchor. Do not
  // send messages, initialize classes, or dereference Objective-C objects
  // while TSan is starting. Normalize only a diagnostic copy of the code PC.
#if defined(TSAN_HAS_OBJC)
  TsanObjCImagePlacement placement = {};
  if (!__tsan_objc_query_image(&placement)) {
#else
  const uptr pc = STRIP_PAC_PC(&class_getImageName);
  Dl_info info = {};
  if (!pc || !dladdr(reinterpret_cast<const void *>(pc), &info) ||
      !info.dli_fbase || !info.dli_fname) {
#endif
    VPrintf(2, "ThreadSanitizer: Objective-C image placement: unavailable\n");
    return;
  }

#if defined(TSAN_HAS_OBJC)
  const uptr pc = placement.pc;
  const uptr base = placement.base;
  const char *image_name = placement.image_name;
  VPrintf(2, "ThreadSanitizer: native Objective-C ABI: BOOL=%s size=%zu\n",
          placement.bool_encoding, placement.bool_size);
#else
  const uptr base = reinterpret_cast<uptr>(info.dli_fbase);
  const char *image_name = info.dli_fname;
#endif
  if (pc < base) {
    VPrintf(2, "ThreadSanitizer: Objective-C image placement: invalid offset\n");
    return;
  }

  // This is an offset from a loaded image base, not the Mach-O ASLR slide.
  // These checks concern the TSan application layout only, not page
  // protections or the validity of every segment in the image.
  VPrintf(2,
          "ThreadSanitizer: Objective-C image placement: %s "
          "image=%s base=%p pc=%p image_offset=0x%zx\n",
          IsValidAppMemoryRange(base, 1) && IsValidAppMemoryRange(pc, 1)
              ? "supported"
              : "outside application memory",
          image_name, reinterpret_cast<void *>(base),
          reinterpret_cast<void *>(pc), pc - base);
}

}  // namespace __tsan

#endif  // SANITIZER_APPLE && !SANITIZER_GO
