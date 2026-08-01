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
  if (cudaHostAlloc(&HostAllocPtr, sizeof(int), cudaHostAllocDefault) !=
      cudaSuccess)
    return 1;

  *HostAllocPtr = 17;
  printf("cudaHostAlloc value: %d\n", *HostAllocPtr);
  // CHECK: cudaHostAlloc value: 17

  if (cudaFreeHost(HostAllocPtr) != cudaSuccess)
    return 1;

  int *MallocHostPtr = nullptr;
  if (cudaMallocHost(&MallocHostPtr, sizeof(int)) != cudaSuccess)
    return 1;

  *MallocHostPtr = 23;
  printf("cudaMallocHost value: %d\n", *MallocHostPtr);
  // CHECK: cudaMallocHost value: 23

  if (cudaFreeHost(MallocHostPtr) != cudaSuccess)
    return 1;
}
