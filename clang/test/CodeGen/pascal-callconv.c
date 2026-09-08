// RUN: %clang_cc1 -triple i386-unknown-windows-msvc -emit-llvm -o - %s | FileCheck %s

int __pascal pascal_keyword(int a, int b) {
  return a + b;
}

int __attribute__((pascal)) pascal_attribute(int a, int b) {
  return a - b;
}

// CHECK-LABEL: define{{.*}} cc 128 i32 @pascal_keyword
// CHECK-LABEL: define{{.*}} cc 128 i32 @pascal_attribute
