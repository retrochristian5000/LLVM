// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -emit-llvm -o - %s \
// RUN: | FileCheck %s

// Test that vector types maintain their required alignment for correctness
// (movaps/movapd instructions will fault on misaligned addresses), even when
// #pragma pack(8) would normally reduce it. This issue affects Windows MSVC
// targets where #pragma pack is commonly used (e.g., MSVC STL).

typedef float v4f32 __attribute__((vector_size(16)));
typedef float v8f32 __attribute__((vector_size(32)));

struct VectorKlass {
  v4f32 v;
};

// CHECK-LABEL: define {{.*}} @{{.*}}test_vector_klass
// CHECK: %v = alloca %struct.VectorKlass, align 16
void test_vector_klass() {
  VectorKlass v;
  v.v = (v4f32){0.0f, 0.0f, 0.0f, 0.0f};
}

#pragma pack(push, 8)
struct PackedVectorKlass {
  v4f32 v;
};

struct PackedLargeVectorKlass {
  v8f32 v;
};

// CHECK-LABEL: define {{.*}} @{{.*}}test_packed_vector
// CHECK: %pv = alloca %struct.PackedVectorKlass, align 16
void test_packed_vector() {
  PackedVectorKlass pv;
  pv.v = (v4f32){0.0f, 0.0f, 0.0f, 0.0f};
}

// CHECK-LABEL: define {{.*}} @{{.*}}test_packed_large_vector
// CHECK: %plv = alloca %struct.PackedLargeVectorKlass, align 32
void test_packed_large_vector() {
  PackedLargeVectorKlass plv;
  plv.v = (v8f32){0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
}

struct InnerWithVector {
  v4f32 v;
};

struct OuterWithVector {
  InnerWithVector inner;
};

// CHECK-LABEL: define {{.*}} @{{.*}}test_nested_vector
// CHECK: %outer = alloca %struct.OuterWithVector, align 16
void test_nested_vector() {
  OuterWithVector outer;
  outer.inner.v = (v4f32){0.0f, 0.0f, 0.0f, 0.0f};
}

// Test array of structs containing vectors
template<typename T, unsigned N>
struct array {
  T _Elems[N];
};

// CHECK-LABEL: define {{.*}} @{{.*}}test_vector_array_packed
// CHECK: %varr = alloca %struct.array, align 16
void test_vector_array_packed() {
  array<PackedVectorKlass, 4> varr;
  for (int i = 0; i < 4; i++)
    varr._Elems[i].v = (v4f32){0.0f, 0.0f, 0.0f, 0.0f};
}
#pragma pack(pop)

struct __attribute__((packed)) ExplicitlyPackedVector {
  v4f32 v;
};

// CHECK-LABEL: define {{.*}} @{{.*}}test_explicitly_packed_vector
// CHECK: %epv = alloca %struct.ExplicitlyPackedVector, align 1
void test_explicitly_packed_vector() {
  ExplicitlyPackedVector epv;
  epv.v = (v4f32){0.0f, 0.0f, 0.0f, 0.0f};
}

#pragma pack(push, 8)
struct FieldPackedVector {
  v4f32 v __attribute__((packed));
};
#pragma pack(pop)

// CHECK-LABEL: define {{.*}} @{{.*}}test_field_packed_vector
// CHECK: %fpv = alloca %struct.FieldPackedVector, align 1
void test_field_packed_vector() {
  FieldPackedVector fpv;
  fpv.v = (v4f32){0.0f, 0.0f, 0.0f, 0.0f};
}

#pragma pack(push, 8)
struct alignas(16) ExplicitAlignedInner {
  long double x;
};

struct OuterWithExplicitAligned {
  ExplicitAlignedInner inner;
};

struct ImplicitAlignedInner {
  long double x;
};                                                                          

struct OuterWithImplicitAligned {
  ImplicitAlignedInner inner;
};
#pragma pack(pop)
                               
// CHECK-FP80-LABEL: define {{.*}} @{{.*}}test_explicit_aligned_nested
// CHECK-FP80: %outer = alloca %struct.OuterWithExplicitAligned, align 16
void test_explicit_aligned_nested() {
  OuterWithExplicitAligned outer;
  outer.inner.x = 0.0L;
}

// CHECK-FP80-LABEL: define {{.*}} @{{.*}}test_implicit_aligned_nested
// CHECK-FP80: %outer = alloca %struct.OuterWithImplicitAligned, align 16
void test_implicit_aligned_nested() {
  OuterWithImplicitAligned outer;
  outer.inner.x = 0.0L;
}
