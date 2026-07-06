// RUN: %clang_cc1 -verify -fopenmp -fopenmp-version=52 -std=c++11 \
// RUN: -ast-dump %s | FileCheck %s

// expected-no-diagnostics 

// CHECK-LABEL: FunctionDecl {{.*}} test_runtime_condition
// CHECK: OMPMetaDirective {{.*}} variants=2
// CHECK-NEXT: IfStmt {{.*}} has_else
// CHECK-NEXT: DeclRefExpr {{.*}} 'int' lvalue ParmVar {{.*}} 'flag' 'int'
// CHECK-NEXT: OMPParallelDirective
// CHECK-NEXT: CapturedStmt
// CHECK-NEXT: CapturedDecl
// CHECK-NEXT: CompoundStmt
// CHECK-NEXT: DeclStmt
// CHECK-NEXT: VarDecl {{.*}} x 'int'
// CHECK: CompoundStmt
// CHECK-NEXT: DeclStmt
void test_runtime_condition(int flag) {
  #pragma omp metadirective when(user={condition(flag)}: parallel) otherwise()
  {
    int x = 0;
  }
}

// CHECK-LABEL: FunctionDecl {{.*}} test_both_nothing
// CHECK-NOT: IfStmt
// CHECK: CompoundStmt
// CHECK: DeclStmt
// CHECK-NEXT: VarDecl {{.*}} x 'int'
void test_both_nothing(int flag) {
#pragma omp metadirective when(user={condition(flag)}: nothing) otherwise()
  {
    int x = 0;
  }
}
