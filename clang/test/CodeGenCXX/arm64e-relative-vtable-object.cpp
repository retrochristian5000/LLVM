// REQUIRES: aarch64-registered-target
//
// Arm64e requires authenticated 64-bit C++ vtable slots. Relative-vtable
// entries are 32-bit offsets, so do not crash or emit a mislabeled object.
//
// RUN: not %clang -target arm64e-apple-macos13 -std=c++20 \
// RUN:   -fexperimental-relative-c++-abi-vtables -c %s -o %t-invalid.o 2>&1 | \
// RUN:   FileCheck %s --check-prefix=REJECT
// RUN: not %clang_cc1 -triple arm64e-apple-macos13 -std=c++20 \
// RUN:   -fexperimental-relative-c++-abi-vtables -emit-obj \
// RUN:   -o %t-invalid-cc1.o %s 2>&1 | FileCheck %s --check-prefix=REJECT
// RUN: %clang -target arm64e-apple-macos13 -std=c++20 \
// RUN:   -fno-experimental-relative-c++-abi-vtables -c %s -o %t-auth.o
// RUN: llvm-objdump --macho --reloc %t-auth.o | FileCheck %s --check-prefix=AUTH
// RUN: %clang -target arm64-apple-macos13 -std=c++20 -c %s -o %t-arm64.o
// RUN: llvm-objdump --macho --reloc %t-arm64.o | FileCheck %s --check-prefix=PLAIN
//
// REJECT: error: unsupported option '-fexperimental-relative-c++-abi-vtables' for target '{{.*}}'
// AUTH: Relocation information (__DATA,__const)
// AUTH: {{(11 \(\?\)|AUTHENTICATED_POINTER)}} {{.*}} __ZN6Widget6methodEv
// PLAIN: Relocation information (__DATA,__const)
// PLAIN: UNSIGND {{.*}} __ZN6Widget6methodEv

struct Widget {
  virtual int method();
  virtual ~Widget();
};

int Widget::method() { return 1; }
Widget::~Widget() = default;

int invoke(Widget *obj) { return obj->method(); }
