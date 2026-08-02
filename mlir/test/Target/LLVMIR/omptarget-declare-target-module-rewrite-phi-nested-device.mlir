// RUN: mlir-translate -mlir-to-llvmir %s | FileCheck %s

// A more complicated exercise of the declare target global use rewrite when
//  the original global is consumed by multiple PHI nodes, some nested, and
//  additionally the original global lives in a non-default address space
//  (address space 2)

module attributes {llvm.target_triple = "amdgcn-amd-amdhsa", omp.is_target_device = true} {
  // CHECK-DAG: @_QMtest_0Esp_decl_tgt_ref_ptr = weak global ptr null, align 8
  llvm.mlir.global external @_QMtest_0Esp() {addr_space = 2 : i32, omp.declare_target = #omp.declaretarget<device_type = (any), capture_clause = (link)>} : i32 {
    %0 = llvm.mlir.constant(0 : i32) : i32
    llvm.return %0 : i32
  }

  // CHECK-LABEL: define hidden void @_QMtest_0Puse_global_nested
  //
  // CHECK: %[[L0:.*]] = load ptr, ptr @_QMtest_0Esp_decl_tgt_ref_ptr, align 8
  // CHECK: br i1 %{{.*}}, label %[[A:.*]], label %[[B:.*]]
  // CHECK: [[A]]:
  // CHECK: %[[PA:.*]] = phi ptr [ %[[L0]], %[[ENTRY:.*]] ]
  // CHECK: br label %[[M1:.*]]
  // CHECK: [[B]]:
  // CHECK: %[[PB:.*]] = phi ptr [ %[[L0]], %[[ENTRY]] ]
  // CHECK: br label %[[M1]]
  // CHECK: [[M1]]:
  // CHECK: %[[PHI1:.*]] = phi ptr [ %[[PB]], %[[B]] ], [ %[[PA]], %[[A]] ]
  //
  // CHECK: %[[L1:.*]] = load ptr, ptr @_QMtest_0Esp_decl_tgt_ref_ptr, align 8
  // CHECK: br i1 %{{.*}}, label %[[C:.*]], label %[[D:.*]]
  // CHECK: [[C]]:
  // CHECK: %[[PC:.*]] = phi ptr [ %[[PHI1]], %[[M1]] ]
  // CHECK: br label %[[M2:.*]]
  // CHECK: [[D]]:
  // CHECK: %[[PD:.*]] = phi ptr [ %[[L1]], %[[M1]] ]
  // CHECK: br label %[[M2]]
  // CHECK: [[M2]]:
  // CHECK: %[[PHI2:.*]] = phi ptr [ %[[PD]], %[[D]] ], [ %[[PC]], %[[C]] ]
  // CHECK: store i32 3, ptr %[[PHI2]], align 4
  llvm.func @_QMtest_0Puse_global_nested(%cond1 : i1, %cond2 : i1) attributes {omp.declare_target = #omp.declaretarget<device_type = (any), capture_clause = (enter)>} {
    %g = llvm.mlir.addressof @_QMtest_0Esp : !llvm.ptr<2>
    %gc = llvm.addrspacecast %g : !llvm.ptr<2> to !llvm.ptr
    llvm.cond_br %cond1, ^bb1(%gc : !llvm.ptr), ^bb2(%gc : !llvm.ptr)
  ^bb1(%a : !llvm.ptr):
    llvm.br ^merge1(%a : !llvm.ptr)
  ^bb2(%b : !llvm.ptr):
    llvm.br ^merge1(%b : !llvm.ptr)
  ^merge1(%m1 : !llvm.ptr):
    %g2 = llvm.mlir.addressof @_QMtest_0Esp : !llvm.ptr<2>
    %g2c = llvm.addrspacecast %g2 : !llvm.ptr<2> to !llvm.ptr
    llvm.cond_br %cond2, ^bb3(%m1 : !llvm.ptr), ^bb4(%g2c : !llvm.ptr)
  ^bb3(%c : !llvm.ptr):
    llvm.br ^merge2(%c : !llvm.ptr)
  ^bb4(%d : !llvm.ptr):
    llvm.br ^merge2(%d : !llvm.ptr)
  ^merge2(%m2 : !llvm.ptr):
    %v = llvm.mlir.constant(3 : i32) : i32
    llvm.store %v, %m2 : i32, !llvm.ptr
    llvm.return
  }
}
