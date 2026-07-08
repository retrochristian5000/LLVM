// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -mlong-double-80 -emit-llvm -o - %s | FileCheck %s

// Test that x86_fp80 (long double with /Qlong-double flag) maintains
// 16-byte alignment for correctness (required by movaps instructions),
// even when #pragma pack would normally reduce it.

struct Klass {
  long double a;
};

// Simulate std::array without including headers
template<typename T, unsigned N>
struct array {
  T _Elems[N];
};

// CHECK-LABEL: define {{.*}} @{{.*}}test_single_klass
// CHECK: %k = alloca %struct.Klass, align 16
// CHECK-NOT: align 8
// CHECK: store x86_fp80 {{.*}}, ptr {{.*}}, align 16
void test_single_klass() {
  Klass k;
  k.a = 0.0L;
}

// CHECK-LABEL: define {{.*}} @{{.*}}test_struct_array
// CHECK: %matrix = alloca %struct.array, align 16
// CHECK-NOT: align 8
// CHECK: store x86_fp80 {{.*}}, ptr {{.*}}, align 16
void test_struct_array() {
  array<Klass, 16> matrix;
  for (int i = 0; i < 16; i++)
    matrix._Elems[i].a = 0.0L;
}

// CHECK-LABEL: define {{.*}} @{{.*}}test_direct_array
// CHECK: %arr = alloca [16 x %struct.Klass], align 16
void test_direct_array() {
  Klass arr[16];
  for (int i = 0; i < 16; i++)
    arr[i].a = 0.0L;
}

// Test with explicit pragma pack(8) - should still maintain 16-byte alignment
#pragma pack(push, 8)
struct PackedKlass {
  long double b;
};

// CHECK-LABEL: define {{.*}} @{{.*}}test_explicit_pack
// CHECK: %pk = alloca %struct.PackedKlass, align 16
// CHECK-NOT: align 8
// CHECK: store x86_fp80 {{.*}}, ptr {{.*}}, align 16
void test_explicit_pack() {
  PackedKlass pk;
  pk.b = 0.0L;
}
#pragma pack(pop)
