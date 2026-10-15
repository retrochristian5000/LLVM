// RUN: %clang_cc1 -triple arm64ec-windows-msvc -fms-extensions -fno-common -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple aarch64-windows-msvc -fms-extensions -fno-common -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-windows-msvc -fms-extensions -fno-common -emit-llvm -o - %s | FileCheck %s
//
// C tentative definitions use bss_seg, including push/pop.  The IR marker
// distinguishes true uninitialized COFF data from explicit data_seg content.

int default_bss;
#pragma bss_seg(".whp$bss")
int outer_bss;
#pragma bss_seg(push, scoped, ".whp$scoped")
int inner_bss;
#pragma bss_seg(pop, scoped)
int restored_bss;
#pragma bss_seg()
int reset_bss;

#pragma data_seg(".whp$data")
int initialized_data = 42;
#pragma data_seg()

// CHECK-DAG: @default_bss = {{.*}}global i32 0, align 4
// CHECK-DAG: @outer_bss = {{.*}}global i32 0, section ".whp$bss", align 4 #[[OUT:[0-9]+]]
// CHECK-DAG: @inner_bss = {{.*}}global i32 0, section ".whp$scoped", align 4 #[[IN:[0-9]+]]
// CHECK-DAG: @restored_bss = {{.*}}global i32 0, section ".whp$bss", align 4 #[[OUT]]
// CHECK-DAG: @reset_bss = {{.*}}global i32 0, align 4
// CHECK-DAG: @initialized_data = {{.*}}global i32 42, section ".whp$data", align 4
// CHECK-DAG: attributes #[[OUT]] = { "bss-section"=".whp$bss" }
// CHECK-DAG: attributes #[[IN]] = { "bss-section"=".whp$scoped" }
