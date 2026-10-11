// Verify the implicit novtable semantics of Microsoft's __interface.
// MicrosoftCXXABI must not initialize the interface's own vfptr, just as for
// an explicitly __declspec(novtable) abstract class.
// RUN: %clang_cc1 -triple i386-pc-win32 -std=c++11 -fms-extensions -fno-rtti -emit-llvm -o - %s | FileCheck %s --implicit-check-not='@"??_7IContract@@6B@"'
// RUN: %clang_cc1 -triple x86_64-pc-win32 -std=c++11 -fms-extensions -fno-rtti -emit-llvm -o - %s | FileCheck %s --implicit-check-not='@"??_7IContract@@6B@"'
// RUN: %clang_cc1 -triple aarch64-pc-windows-msvc -std=c++11 -fms-extensions -fno-rtti -emit-llvm -o - %s | FileCheck %s --implicit-check-not='@"??_7IContract@@6B@"'
// RUN: %clang_cc1 -triple i386-pc-win32 -std=c++2c -fms-extensions -fno-rtti -emit-llvm -o - %s | FileCheck %s --implicit-check-not='@"??_7IContract@@6B@"'
// RUN: %clang_cc1 -triple x86_64-pc-win32 -std=c++2c -fms-extensions -fno-rtti -emit-llvm -o - %s | FileCheck %s --implicit-check-not='@"??_7IContract@@6B@"'
// RUN: %clang_cc1 -triple aarch64-pc-windows-msvc -std=c++2c -fms-extensions -fno-rtti -emit-llvm -o - %s | FileCheck %s --implicit-check-not='@"??_7IContract@@6B@"'

__interface IContract {
  int get();
};

struct __declspec(novtable) IExplicit {
  virtual int query() = 0;
};

struct Derived : IContract {
  int get() override { return 7; }
};

struct ExplicitDerived : IExplicit {
  int query() override { return 8; }
};

// Force out-of-line constructors and base-subobject initialization.
int use() {
  Derived d;
  ExplicitDerived e;
  return d.get() + e.query();
}

// The concrete derived class vftables must still be emitted.
// CHECK-DAG: @"??_7Derived@@6B@" =
// CHECK-DAG: @"??_7ExplicitDerived@@6B@" =
// Neither abstract base should acquire its own emitted vftable.
// CHECK-NOT: @"??_7IExplicit@@6B@"
