/*===---- hip_runtime.cpp - HIP runtime api implementations ----------------===
 *
 * Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */

#include "hip_runtime.h"

#include "LanguageLaunch.h"
#include "OffloadAPI.h"

#define LANGUAGE hip

#include "../../kernel/src/LanguageRuntime.cpp"

extern "C" {
#define HIP_LAUNCH_KERNEL(SUFFIX)                                              \
  hipError_t hipLaunchKernel##SUFFIX(const char *KernelID, dim3 GridDim,       \
                                     dim3 BlockDim, void *KernelArgsPtr,       \
                                     size_t DynamicSharedMem, void *Stream) {  \
    return LastError = convertResult(__llvmLaunchKernelImpl(                   \
               KernelID, GridDim, BlockDim, KernelArgsPtr, DynamicSharedMem,   \
               Stream));                                                       \
  }

HIP_LAUNCH_KERNEL()
HIP_LAUNCH_KERNEL(_spt)
HIP_LAUNCH_KERNEL(_ptsz)
#undef HIP_LAUNCH_KERNEL
}
