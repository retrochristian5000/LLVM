//===------- Types.h - Kernel Language (CUDA/HIP) api types ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstddef>
#include <cstdint>

#ifdef __CLANG_GPU_BUILTIN_VARS_H__
using uint3 = dim3;
#else
struct uint3 {
  unsigned x = 0, y = 0, z = 0;
};

struct dim3 : uint3 {
  constexpr dim3(unsigned X = 1, unsigned Y = 1, unsigned Z = 1)
      : uint3{X, Y, Z} {}
  constexpr dim3(uint3 V) : uint3{V.x, V.y, V.z} {}
};
#endif

struct CallConfigurationTy {
  dim3 GridSize;
  dim3 BlockSize;
  size_t SharedMemory;
  void *Stream;
};
