//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBSYCL_CONSUME_BINARY_ERROR
#define _LIBSYCL_CONSUME_BINARY_ERROR

#include <sycl/__impl/detail/config.hpp>

#include <llvm/Frontend/Offloading/Utility.h>
#include <llvm/Object/OffloadBinary.h>

// Libsycl is built with RTTI while LLVM is not, which would normally cause this
// function to break debug builds. To address that, its definition is placed in
// a separate source file that's compiled without RTTI.
_LIBSYCL_BEGIN_NAMESPACE_SYCL
namespace detail {
void consumeBinaryError(
    llvm::Expected<
        llvm::SmallVector<std::unique_ptr<llvm::object::OffloadBinary>>>
        &BinOrErr);
} // namespace detail
_LIBSYCL_END_NAMESPACE_SYCL
#endif // _LIBSYCL_CONSUME_BINARY_ERROR
