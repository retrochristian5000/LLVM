//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <detail/consume_binary_error.hpp>

_LIBSYCL_BEGIN_NAMESPACE_SYCL
namespace detail {
void consumeBinaryError(
    llvm::Expected<
        llvm::SmallVector<std::unique_ptr<llvm::object::OffloadBinary>>>
        &BinOrErr) {
  llvm::consumeError(BinOrErr.takeError());
}
} // namespace detail
_LIBSYCL_END_NAMESPACE_SYCL
