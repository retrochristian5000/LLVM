// RUN: %clang_cc1 -verify -fopenmp -fopenmp-version=52 -std=c++11 \
// RUN: -triple x86_64-unknown-linux -emit-llvm %s -o - | FileCheck %s

// expected-no-diagnostics

// Test runtime selection with non-constant user condition
// Runtime metadirectives are transformed to if-else in Sema

void test_runtime_condition(int flag) {
// CHECK-LABEL: define {{.*}} void @_Z22test_runtime_conditioni(i32 noundef %flag)
// CHECK: %flag.addr = alloca i32, align 4
// CHECK: %x = alloca i32, align 4
// CHECK: store i32 %flag, ptr %flag.addr, align 4
// CHECK: [[LOAD:%.*]] = load i32, ptr %flag.addr, align 4
// CHECK: [[TOBOOL:%.*]] = icmp ne i32 [[LOAD]], 0
// CHECK: br i1 [[TOBOOL]], label %if.then, label %if.else
// CHECK: if.then:
// CHECK: call void (ptr, i32, ptr, ...) @__kmpc_fork_call(ptr @{{[0-9]+}}, i32 0, ptr @_Z22test_runtime_conditioni.omp_outlined)
// CHECK: br label %if.end
// CHECK: if.else:
// CHECK: store i32 0, ptr %x, align 4
// CHECK: br label %if.end
// CHECK: if.end:
// CHECK: ret void
  #pragma omp metadirective when(user={condition(flag)}: parallel) otherwise()
  {
    int x = 0;
  }
}
// CHECK: define internal void @_Z22test_runtime_conditioni.omp_outlined(ptr {{.*}}, ptr {{.*}})

void test_runtime_condition_two_variants(int flag) {
// CHECK-LABEL: define {{.*}} void @_Z35test_runtime_condition_two_variantsi(i32 noundef %flag)
// CHECK: %flag.addr = alloca i32, align 4
// CHECK: %i = alloca i32, align 4
// CHECK: store i32 %flag, ptr %flag.addr, align 4
// CHECK: [[LOAD:%.*]] = load i32, ptr %flag.addr, align 4
// CHECK: [[TOBOOL:%.*]] = icmp ne i32 [[LOAD]], 0
// CHECK: br i1 [[TOBOOL]], label %if.then, label %if.else
// CHECK: if.then:
// CHECK: call void (ptr, i32, ptr, ...) @__kmpc_fork_call(ptr @{{[0-9]+}}, i32 0, ptr @_Z35test_runtime_condition_two_variantsi.omp_outlined)
// CHECK: br label %if.end
// CHECK: if.else:
// CHECK: store i32 0, ptr %i, align 4
// CHECK: br label %for.cond
// CHECK: for.cond:
// CHECK: [[LOAD_I:%.*]] = load i32, ptr %i, align 4
// CHECK: [[CMP:%.*]] = icmp slt i32 [[LOAD_I]], 10
// CHECK: br i1 [[CMP]], label %for.body, label %for.end
// CHECK: for.body:
// CHECK: br label %for.inc
// CHECK: for.inc:
// CHECK: [[LOAD_I2:%.*]] = load i32, ptr %i, align 4
// CHECK: [[INC:%.*]] = add nsw i32 [[LOAD_I2]], 1
// CHECK: store i32 [[INC]], ptr %i, align 4
// CHECK: br label %for.cond
// CHECK: for.end:
// CHECK: br label %if.end
// CHECK: if.end:
// CHECK: ret void
  #pragma omp metadirective when(user={condition(flag)}: parallel) otherwise(parallel for)
  for (int i = 0; i < 10; i++)
    ;
}
// CHECK: define internal void @_Z35test_runtime_condition_two_variantsi.omp_outlined(ptr {{.*}}, ptr {{.*}})

// Test with clauses on the parallel directive
void test_runtime_with_clauses(int flag, int n) {
// CHECK-LABEL: define {{.*}}void @_Z25test_runtime_with_clausesii(i32 noundef %flag, i32 noundef %n)
// CHECK: %flag.addr = alloca i32, align 4
// CHECK: %n.addr = alloca i32, align 4
// CHECK: store i32 %flag, ptr %flag.addr, align 4
// CHECK: store i32 %n, ptr %n.addr, align 4
// CHECK: [[LOAD:%.*]] = load i32, ptr %flag.addr, align 4
// CHECK: [[TOBOOL:%.*]] = icmp ne i32 [[LOAD]], 0
// CHECK: br i1 [[TOBOOL]], label %if.then, label %if.else
// CHECK: if.then:
// CHECK: call void (ptr, i32, ptr, ...) @__kmpc_fork_call
// CHECK: br label %if.end
// CHECK: if.else:
// CHECK: br label %if.end
// CHECK: if.end:
// CHECK: ret void
#pragma omp metadirective when(user={condition(flag)}: parallel num_threads(n)) otherwise()
  {
    int x = 0;
  }
}
// CHECK: define internal void @_Z25test_runtime_with_clausesii.omp_outlined(ptr {{.*}}, ptr {{.*}})

  void test_runtime_multiple_stmts(int flag) {
// CHECK-LABEL: define {{.*}}void @_Z27test_runtime_multiple_stmtsi(i32 noundef %flag)
// CHECK: [[LOAD:%.*]] = load i32, ptr %flag.addr, align 4
// CHECK: [[TOBOOL:%.*]] = icmp ne i32 [[LOAD]], 0
// CHECK: br i1 [[TOBOOL]], label %if.then, label %if.else
// CHECK: if.then:
// CHECK: call void (ptr, i32, ptr, ...) @__kmpc_fork_call
// CHECK: br label %if.end
// CHECK: if.else:
// CHECK: store i32 1, ptr %x
// CHECK: store i32 2, ptr %y
// CHECK: store i32 3, ptr %z
// CHECK: br label %if.end
// CHECK: if.end:
// CHECK: ret void
  #pragma omp metadirective when(user={condition(flag)}: parallel) otherwise()
  {
    int x = 1;
    int y = 2;
    int z = 3;
  }
}
// CHECK:define internal void @_Z27test_runtime_multiple_stmtsi.omp_outlined(ptr {{.*}}, ptr {{.*}})

// Test that compile-time constant condition still works (no if-else generated)
void test_compile_time_constant(void) {
// CHECK-LABEL: define {{.*}}void @_Z26test_compile_time_constantv()
// CHECK-NOT: br i1
// CHECK: call void (ptr, i32, ptr, ...) @__kmpc_fork_call
// CHECK: ret void
  #pragma omp metadirective when(user={condition(1)}: parallel) otherwise()
  {
    int x = 0;
  }
}
// CHECK: define internal void @_Z26test_compile_time_constantv.omp_outlined(ptr {{.*}}, ptr {{.*}})

// Test when both branches are "nothing" - no if-else is generated
void test_runtime_otherwise(int flag) {
// CHECK-LABEL: define {{.*}} @_Z22test_runtime_otherwisei
// CHECK: %flag.addr = alloca i32
// CHECK: %x = alloca i32
// CHECK: store i32 %flag, ptr %flag.addr
// CHECK: store i32 0, ptr %x
// CHECK: ret void
  #pragma omp metadirective when(user={condition(flag)}: nothing) otherwise()
  {
    int x = 0;
  }
}
