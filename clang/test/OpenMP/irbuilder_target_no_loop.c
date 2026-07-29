// REQUIRES: amdgpu-registered-target

// RUN: %clang_cc1 -verify -fopenmp -x c -triple x86_64-unknown-linux-gnu \
// RUN:   -fopenmp-targets=amdgcn-amd-amdhsa -emit-llvm-bc %s -o %t-host.bc

// RUN: %clang_cc1 -verify -fopenmp -x c -triple amdgcn-amd-amdhsa \
// RUN:   -fopenmp-targets=amdgcn-amd-amdhsa -fopenmp-is-target-device \
// RUN:   -fopenmp-host-ir-file-path %t-host.bc \
// RUN:   -fopenmp-assume-teams-oversubscription \
// RUN:   -fopenmp-assume-threads-oversubscription \
// RUN:   -fopenmp-enable-irbuilder -emit-llvm %s -o - | FileCheck %s \
// RUN:   --check-prefixes=CHECK,IRB \
// RUN:   --implicit-check-not=__kmpc_distribute_static_init \
// RUN:   --implicit-check-not=__kmpc_for_static_init

// RUN: %clang_cc1 -verify -fopenmp -x c -triple amdgcn-amd-amdhsa \
// RUN:   -fopenmp-targets=amdgcn-amd-amdhsa -fopenmp-is-target-device \
// RUN:   -fopenmp-host-ir-file-path %t-host.bc \
// RUN:   -fopenmp-assume-teams-oversubscription \
// RUN:   -fopenmp-assume-threads-oversubscription \
// RUN:   -emit-llvm %s -o - | FileCheck %s \
// RUN:   --check-prefixes=CHECK,NOIRB \
// RUN:   --implicit-check-not=__kmpc_distribute_for_static_loop

// expected-no-diagnostics

void no_loop(int *array) {
#pragma omp target teams distribute parallel for
  for (int i = 0; i < 1024; ++i)
    array[i] = i + 1;
}

// The third field is OMP_TGT_EXEC_MODE_SPMD_NO_LOOP (1 << 2 | 1 << 1).
// CHECK: @{{.*}}no_loop{{.*}}_kernel_environment = weak_odr protected addrspace(1) constant %struct.KernelEnvironmentTy { %struct.ConfigurationEnvironmentTy { i8 0, i8 1, i8 6,

// The trailing i8 is the NoLoop flag, forwarded to the device runtime as
// one_iteration_per_thread.
// IRB: call void @__kmpc_distribute_for_static_loop_4u({{.*}}, i32 0, i32 0, i8 1)

// NOIRB: call void @__kmpc_distribute_static_init_4(
// NOIRB: call void @__kmpc_for_static_init_4(
