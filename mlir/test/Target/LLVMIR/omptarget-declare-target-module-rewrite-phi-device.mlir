// RUN: mlir-translate -mlir-to-llvmir %s | FileCheck %s

// This test verifies the declare target global use rewrite mechanism when
// the original global is consumed by a PHI node. Making sure we rewrite this
// case correctly.

module attributes {llvm.target_triple = "amdgcn-amd-amdhsa", omp.is_target_device = true} {
  // CHECK-DAG: @_QMtest_0Esp_decl_tgt_ref_ptr = weak global ptr null, align 8
  llvm.mlir.global external @_QMtest_0Esp() {addr_space = 0 : i32, omp.declare_target = #omp.declaretarget<device_type = (any), capture_clause = (link)>} : i32 {
    %0 = llvm.mlir.constant(0 : i32) : i32
    llvm.return %0 : i32
  }

  // CHECK-LABEL: define hidden void @_QMtest_0Puse_global
  // CHECK: %[[LOAD0:.*]] = load ptr, ptr @_QMtest_0Esp_decl_tgt_ref_ptr, align 8
  // CHECK: %[[LOAD1:.*]] = load ptr, ptr @_QMtest_0Esp_decl_tgt_ref_ptr, align 8
  // CHECK: br i1 %{{.*}}, label %[[BB_A:.*]], label %[[BB_B:.*]]
  // CHECK: [[BB_A]]:
  // CHECK: %[[PHI_A:.*]] = phi ptr [ %[[LOAD1]], %{{.*}} ]
  // CHECK: br label %[[MERGE:.*]]
  // CHECK: [[BB_B]]:
  // CHECK: %[[PHI_B:.*]] = phi ptr [ %[[LOAD0]], %{{.*}} ]
  // CHECK: br label %[[MERGE]]
  // CHECK: [[MERGE]]:
  // CHECK: %[[PHI:.*]] = phi ptr [ %[[PHI_B]], %[[BB_B]] ], [ %[[PHI_A]], %[[BB_A]] ]
  // CHECK: store i32 2, ptr %[[PHI]], align 4
  llvm.func @_QMtest_0Puse_global(%cond : i1) attributes {omp.declare_target = #omp.declaretarget<device_type = (any), capture_clause = (enter)>} {
    %0 = llvm.mlir.addressof @_QMtest_0Esp : !llvm.ptr
    llvm.cond_br %cond, ^bb1(%0 : !llvm.ptr), ^bb2(%0 : !llvm.ptr)
  ^bb1(%arg1 : !llvm.ptr):
    llvm.br ^bb3(%arg1 : !llvm.ptr)
  ^bb2(%arg2 : !llvm.ptr):
    llvm.br ^bb3(%arg2 : !llvm.ptr)
  ^bb3(%arg3 : !llvm.ptr):
    %1 = llvm.mlir.constant(2 : i32) : i32
    llvm.store %1, %arg3 : i32, !llvm.ptr
    llvm.return
  }
}
