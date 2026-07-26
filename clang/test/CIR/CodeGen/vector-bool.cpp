// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -Wno-unused-value -fclangir -emit-cir %s -o %t.cir
// RUN: FileCheck --input-file=%t.cir %s -check-prefix=CIR
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -Wno-unused-value -fclangir -emit-llvm %s -o %t-cir.ll
// RUN: FileCheck --input-file=%t-cir.ll %s -check-prefix=LLVM
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -Wno-unused-value -emit-llvm %s -o %t.ll
// RUN: FileCheck --input-file=%t.ll %s -check-prefix=OGCG

typedef bool v8b __attribute__((ext_vector_type(8)));

void vec_bool_without_padding_needed() {
  v8b a;
  v8b b;
}

// CIR: %[[A_ADDR:.*]] = cir.alloca "a" {{.*}} : !cir.ptr<!u8i>
// CIR: %[[B_ADDR:.*]] = cir.alloca "b" {{.*}} : !cir.ptr<!u8i>

// LLVM: %[[A_ADDR:.*]] = alloca i8, i64 1, align 1
// LLVM: %[[B_ADDR:.*]] = alloca i8, i64 1, align 1

// OGCG: %[[A_ADDR:.*]] = alloca i8, align 1
// OGCG: %[[B_ADDR:.*]] = alloca i8, align 1
