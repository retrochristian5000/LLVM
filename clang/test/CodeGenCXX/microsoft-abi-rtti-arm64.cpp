// Keep the MSVC RTTI ABI's 32-bit image-relative fields distinct from
// 64-bit pointers on Windows ARM64.  RTTI is shared with the 64-bit x86
// layout, not with the 32-bit MSVC layout or Windows GNU (Itanium) RTTI.
//
// RUN: %clang_cc1 -triple aarch64-pc-windows-msvc -std=c++17 -fms-extensions -emit-llvm -o - %s | FileCheck %s --check-prefix=ARM64
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -std=c++17 -fms-extensions -emit-llvm -o - %s | FileCheck %s --check-prefix=X64
// RUN: %clang_cc1 -triple i686-pc-windows-msvc -std=c++17 -fms-extensions -emit-llvm -o - %s | FileCheck %s --check-prefix=X86
// RUN: %clang_cc1 -triple aarch64-w64-windows-gnu -std=c++17 -fms-extensions -emit-llvm -o - %s | FileCheck %s --check-prefix=GNU
// RUN: %clang -target aarch64-pc-windows-msvc -x c++ -std=c++17 -c %s -o %t.obj
// RUN: llvm-objdump -r %t.obj | FileCheck %s --check-prefix=RELOCS

// Permit typeid without depending on a target Windows C++ standard library.
namespace std {
class type_info {};
}

struct Left {
  virtual ~Left() {}
  virtual int f() { return 1; }
};
struct Right {
  virtual ~Right() {}
  virtual int g() { return 2; }
};
struct Derived : Left, Right {
  int f() override { return 3; }
  int g() override { return 4; }
};

// Force both vftables, including the secondary Right subobject, to be emitted.
Derived global_object;

extern "C" __declspec(dllexport) Derived *cross_cast(Right *p) {
  return dynamic_cast<Derived *>(p);
}
extern "C" __declspec(dllexport) Left *side_cast(Right *p) {
  return dynamic_cast<Left *>(p);
}
extern "C" __declspec(dllexport) void *top_cast(Right *p) {
  return dynamic_cast<void *>(p);
}
extern "C" __declspec(dllexport) const void *static_type() {
  return &typeid(Derived);
}

// ARM64: %rtti.CompleteObjectLocator = type { i32, i32, i32, i32, i32, i32 }
// ARM64-DAG: @"??_R4Derived@@6BLeft@@@" = linkonce_odr constant %rtti.CompleteObjectLocator { i32 1, i32 0, i32 0, i32 trunc {{.*}} @__ImageBase {{.*}}
// ARM64-DAG: @"??_R4Derived@@6BRight@@@" = linkonce_odr constant %rtti.CompleteObjectLocator { i32 1, i32 8, i32 0, i32 trunc {{.*}} @__ImageBase {{.*}}
// ARM64: declare dso_local ptr @__RTDynamicCast(ptr, i32, ptr, ptr, i32)
// ARM64: declare dso_local ptr @__RTCastToVoid(ptr)
// ARM64: ret ptr @"??_R0?AUDerived@@@8"

// X64: %rtti.CompleteObjectLocator = type { i32, i32, i32, i32, i32, i32 }
// X64-DAG: @"??_R4Derived@@6BLeft@@@" = linkonce_odr constant %rtti.CompleteObjectLocator { i32 1, i32 0, i32 0, i32 trunc {{.*}} @__ImageBase {{.*}}
// X64-DAG: @"??_R4Derived@@6BRight@@@" = linkonce_odr constant %rtti.CompleteObjectLocator { i32 1, i32 8, i32 0, i32 trunc {{.*}} @__ImageBase {{.*}}

// X86: %rtti.CompleteObjectLocator = type { i32, i32, i32, ptr, ptr }
// X86-DAG: @"??_R4Derived@@6BLeft@@@" = linkonce_odr constant %rtti.CompleteObjectLocator { i32 0, i32 0, i32 0, ptr @"??_R0?AUDerived@@@8", ptr @"??_R3Derived@@8" }
// X86-DAG: @"??_R4Derived@@6BRight@@@" = linkonce_odr constant %rtti.CompleteObjectLocator { i32 0, i32 4, i32 0, ptr @"??_R0?AUDerived@@@8", ptr @"??_R3Derived@@8" }

// GNU: @_ZTV7Derived = linkonce_odr
// GNU: @_ZTI7Derived = linkonce_odr
// GNU: call ptr @__dynamic_cast(ptr

// RELOCS-DAG: IMAGE_REL_ARM64_ADDR32NB ??_R4Derived@@6BLeft@@@
// RELOCS-DAG: IMAGE_REL_ARM64_ADDR32NB ??_R4Derived@@6BRight@@@
// RELOCS-DAG: IMAGE_REL_ARM64_ADDR32NB ??_R0?AUDerived@@@8
