// clang-format off
// RUN: %clang++ %flags -foffload-via-llvm --offload-arch=native %s -o %t
// RUN: %t | %fcheck-generic
// RUN: %clang++ %flags -foffload-via-llvm --offload-arch=native %s -o %t -fopenmp
// RUN: %t | %fcheck-generic
// clang-format on

// UNSUPPORTED: aarch64-unknown-linux-gnu
// UNSUPPORTED: x86_64-unknown-linux-gnu
// UNSUPPORTED: nvptx64-nvidia-cuda-LTO
// UNSUPPORTED: amdgcn-amd-amdhsa-LTO
// UNSUPPORTED: amdgpu-amd-amdhsa-LTO
// UNSUPPORTED: intelgpu

#include <stdio.h>

int main(int argc, char **argv) {
  int *HostAllocPtr = nullptr;
  if (hipHostAlloc(&HostAllocPtr, sizeof(int), hipHostAllocDefault) !=
      hipSuccess)
    return 1;

  *HostAllocPtr = 17;
  printf("hipHostAlloc value: %d\n", *HostAllocPtr);
  // CHECK: hipHostAlloc value: 17

  if (hipFreeHost(HostAllocPtr) != hipSuccess)
    return 1;

  int *MallocHostPtr = nullptr;
  if (hipMallocHost(&MallocHostPtr, sizeof(int)) != hipSuccess)
    return 1;

  *MallocHostPtr = 23;
  printf("hipMallocHost value: %d\n", *MallocHostPtr);
  // CHECK: hipMallocHost value: 23

  if (hipFreeHost(MallocHostPtr) != hipSuccess)
    return 1;
}
