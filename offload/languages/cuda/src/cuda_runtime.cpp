/*===---- cuda_runtime.cpp - CUDA runtime api implementations --------------===
 *
 * Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */

#include "cuda_runtime.h"

#include "LanguageLaunch.h"
#include "OffloadAPI.h"

#define LANGUAGE cuda

#include "../../kernel/src/LanguageRuntime.cpp"

extern "C" {
#define CUDA_LAUNCH_KERNEL(SUFFIX)                                             \
  cudaError_t cudaLaunchKernel##SUFFIX(                                        \
      const char *KernelID, dim3 GridDim, dim3 BlockDim, void *KernelArgsPtr,  \
      size_t DynamicSharedMem, void *Stream) {                                 \
    return LastError = convertResult(__llvmLaunchKernelImpl(                   \
               KernelID, GridDim, BlockDim, KernelArgsPtr, DynamicSharedMem,   \
               Stream));                                                       \
  }

CUDA_LAUNCH_KERNEL()
CUDA_LAUNCH_KERNEL(_ptsz)
CUDA_LAUNCH_KERNEL(_spt)
#undef CUDA_LAUNCH_KERNEL
}
