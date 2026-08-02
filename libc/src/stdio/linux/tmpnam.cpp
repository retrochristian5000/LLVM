//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Declaration of tmpnam, a POSIX function that generate a string that is a
/// valid pathname that does not name an existing file.
/// See:
/// https://pubs.opengroup.org/onlinepubs/9799919799/functions/tmpnam.html
///
//===----------------------------------------------------------------------===//

#include "src/stdio/tmpnam.h"
#include "hdr/errno_macros.h"
#include "hdr/stdio_macros.h"
#include "hdr/unistd_macros.h"
#include "src/__support/CPP/atomic.h"
#include "src/__support/CPP/string_view.h"
#include "src/__support/OSUtil/linux/syscall_wrappers/access.h"
#include "src/__support/OSUtil/linux/syscall_wrappers/getrandom.h"
#include "src/__support/macros/config.h"
#include "src/string/memory_utils/inline_memcpy.h"

namespace LIBC_NAMESPACE_DECL {

static char tmpbuf[L_tmpnam];
static cpp::Atomic<size_t> tmpnam_budget = TMP_MAX;

// Partially thread-safe:
// - When null is handed it is not thread-safe.
// - We do some work to ensure that cases where we need to use tmpnam_budget
//   that we lock around it.
LLVM_LIBC_FUNCTION(char *, tmpnam, (char *s)) {
  if (s == nullptr)
    s = tmpbuf;

  // here if the s is null then use tmpbuf and if sizeof
  // POSIX portable filename character set, sorted by ASCII value.
  // See
  // https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap03.html#tag_03_265
  const char charset[] = "-.0123456789"
                         "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                         "_"
                         "abcdefghijklmnopqrstuvwxyz";

  // We want to construct: P_tmpdir / <14 random chars> \0
  // P_tmpdir is "/tmp" (length 4).
  // Total length must be L_tmpnam (20).
  // /tmp/ is 5 chars.
  // Random suffix is 14 chars.
  // Null terminator is 1 char.
  // Total: 5 + 14 + 1 = 20.
  constexpr cpp::string_view PREFIX = P_tmpdir "/";
  static_assert(PREFIX.size() + 14 + 1 == L_tmpnam, "L_tmpnam mismatch");
  inline_memcpy(s, PREFIX.data(), PREFIX.size());
  constexpr size_t PREFIX_SIZE = PREFIX.size();
  constexpr size_t SUFFIX_SIZE = 14;

  bool is_unique = false;
  while (!is_unique) {
    size_t curr_budget = tmpnam_budget.load(cpp::MemoryOrder::RELAXED);

    do {
      if (curr_budget == 0)
        break;
    } while (
        !tmpnam_budget.compare_exchange_strong(curr_budget, curr_budget - 1));

    if (curr_budget == 0)
      break;

    uint8_t rand_bytes[L_tmpnam];
    auto ret = linux_syscalls::getrandom(rand_bytes, SUFFIX_SIZE, 0);
    if (!ret.has_value()) {
      /* return nullptr when getrandom fails but consume tmpnam budget */
      return nullptr;
    }

    for (size_t i = 0; i < SUFFIX_SIZE; i++) {
      s[PREFIX_SIZE + i] = charset[rand_bytes[i] % (sizeof(charset) - 1)];
    }
    s[L_tmpnam - 1] = '\0';
    auto res = linux_syscalls::access(s, F_OK);
    is_unique = (!res.has_value() && res.error() == ENOENT);
  }

  if (is_unique)
    return s;
  /* implementation-defined: if we exhaust budget we return nullptr */
  return nullptr;
}
} // namespace LIBC_NAMESPACE_DECL
