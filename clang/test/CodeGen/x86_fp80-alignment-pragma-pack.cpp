// RUN: %clang_cc1 -triple x86_64-pc-windows-gnu -emit-llvm -o - %s \
// RUN: | FileCheck %s --check-prefix=CHECK-FP80

// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -emit-llvm -o - %s \
// RUN: | FileCheck %s --check-prefix=CHECK-VEC

// Test that x86_fp80 (long double) and vector types maintain their required
// alignment for correctness (movaps/movapd instructions will fault on misaligned
// addresses), even when #pragma pack(8) would normally reduce it. This issue
// affects Windows targets where #pragma pack is commonly used.
// Note: GCC on Linux preserves such alignment even with #pragma pack, so this fix
// is Windows-specific to avoid breaking existing Clang ABI on other platforms.
// Note: We use windows-gnu (MinGW) for x86_fp80 tests because windows-msvc doesn't
// support 80-bit long double.

typedef float v4f32 __attribute__((vector_size(16)));
typedef float v8f32 __attribute__((vector_size(32)));

struct Klass {
  long double a;
};

struct VectorKlass {
  v4f32 v;
};

// CHECK-FP80-LABEL: define {{.*}} @{{.*}}test_single_klass
// CHECK-FP80: %k = alloca %struct.Klass, align 16
// CHECK-FP80: store x86_fp80 {{.*}}, ptr {{.*}}, align 16
void test_single_klass() {
  Klass k;
  k.a = 0.0L;
}

// CHECK-VEC-LABEL: define {{.*}} @{{.*}}test_vector_klass
// CHECK-VEC: %v = alloca %struct.VectorKlass, align 16
void test_vector_klass() {
  VectorKlass v;
  v.v = (v4f32){0.0f, 0.0f, 0.0f, 0.0f};
}

// Test with pragma pack(8) - should STILL maintain required alignment
// This is the key test case for the bug fix
#pragma pack(push, 8)

struct PackedKlass {
  long double b;
};

struct PackedVectorKlass {
  v4f32 v;
};

struct PackedLargeVectorKlass {
  v8f32 v;
};

// Simulate std::array without including headers
template<typename T, unsigned N>
struct array {
  T _Elems[N];
};

// CHECK-FP80-LABEL: define {{.*}} @{{.*}}test_explicit_pack
// CHECK-FP80: %pk = alloca %struct.PackedKlass, align 16
// CHECK-FP80: store x86_fp80 {{.*}}, ptr {{.*}}, align 16
void test_explicit_pack() {
  PackedKlass pk;
  pk.b = 0.0L;
}

// CHECK-VEC-LABEL: define {{.*}} @{{.*}}test_packed_vector
// CHECK-VEC: %pv = alloca %struct.PackedVectorKlass, align 16
void test_packed_vector() {
  PackedVectorKlass pv;
  pv.v = (v4f32){0.0f, 0.0f, 0.0f, 0.0f};
}

// CHECK-VEC-LABEL: define {{.*}} @{{.*}}test_packed_large_vector
// CHECK-VEC: %plv = alloca %struct.PackedLargeVectorKlass, align 32
void test_packed_large_vector() {
  PackedLargeVectorKlass plv;
  plv.v = (v8f32){0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
}

// CHECK-FP80-LABEL: define {{.*}} @{{.*}}test_struct_array_packed
// CHECK-FP80: %matrix = alloca %struct.array, align 16
void test_struct_array_packed() {
  array<PackedKlass, 16> matrix;
  for (int i = 0; i < 16; i++)
    matrix._Elems[i].b = 0.0L;
}

// CHECK-FP80-LABEL: define {{.*}} @{{.*}}test_direct_array_packed
// CHECK-FP80: %arr = alloca [16 x %struct.PackedKlass], align 16
void test_direct_array_packed() {
  PackedKlass arr[16];
  for (int i = 0; i < 16; i++)
    arr[i].b = 0.0L;
}

// CHECK-VEC-LABEL: define {{.*}} @{{.*}}test_vector_array_packed
// CHECK-VEC: %varr = alloca %struct.array{{.*}}, align 16
void test_vector_array_packed() {
  array<PackedVectorKlass, 4> varr;
  for (int i = 0; i < 4; i++)
    varr._Elems[i].v = (v4f32){0.0f, 0.0f, 0.0f, 0.0f};
}

#pragma pack(pop)

// Test that __attribute__((packed)) on the struct reduces vector alignment.
// This is needed for unaligned load/store intrinsics like _mm_loadu_ps.
struct __attribute__((packed)) ExplicitlyPackedVector {
  v4f32 v;
};

// CHECK-VEC-LABEL: define {{.*}} @{{.*}}test_explicitly_packed_vector
// CHECK-VEC: %epv = alloca %struct.ExplicitlyPackedVector, align 1
void test_explicitly_packed_vector() {
  ExplicitlyPackedVector epv;
  epv.v = (v4f32){0.0f, 0.0f, 0.0f, 0.0f};
}

#pragma pack(push, 8)
struct FieldPackedVector {
  v4f32 v __attribute__((packed));
};
#pragma pack(pop)

// CHECK-VEC-LABEL: define {{.*}} @{{.*}}test_field_packed_vector
// CHECK-VEC: %fpv = alloca %struct.FieldPackedVector, align 1
void test_field_packed_vector() {
  FieldPackedVector fpv;
  fpv.v = (v4f32){0.0f, 0.0f, 0.0f, 0.0f};
}
