; RUN: not llvm-as -disable-output < %s 2>&1 | FileCheck %s

target triple = "nvptx64-nvidia-cuda"

declare void @llvm.nvvm.cp.async.bulk.reduce.s2g.u32(ptr addrspace(1), ptr addrspace(3), i32, i32 immarg, i32 immarg, i64, i1 immarg)
declare void @llvm.nvvm.cp.async.bulk.reduce.s2g.b32(ptr addrspace(1), ptr addrspace(3), i32, i32 immarg, i32 immarg, i64, i1 immarg)
declare void @llvm.nvvm.cp.async.bulk.reduce.s2g.f16(ptr addrspace(1), ptr addrspace(3), i32, i32 immarg, i32 immarg, i64, i1 immarg)

define void @invalid_u32(ptr addrspace(1) %dst, ptr addrspace(3) %src, i32 %size, i64 %ch) {
  ; CHECK: immarg value 0 out of rangeset
  call void @llvm.nvvm.cp.async.bulk.reduce.s2g.u32(ptr addrspace(1) %dst, ptr addrspace(3) %src, i32 %size, i32 0, i32 2, i64 %ch, i1 0)
  ret void
}

define void @invalid_b32(ptr addrspace(1) %dst, ptr addrspace(3) %src, i32 %size, i64 %ch) {
  ; CHECK: immarg value 3 out of rangeset
  call void @llvm.nvvm.cp.async.bulk.reduce.s2g.b32(ptr addrspace(1) %dst, ptr addrspace(3) %src, i32 %size, i32 3, i32 2, i64 %ch, i1 0)
  ret void
}

define void @invalid_f16(ptr addrspace(1) %dst, ptr addrspace(3) %src, i32 %size, i64 %ch) {
  ; CHECK: immarg value 4 out of rangeset
  call void @llvm.nvvm.cp.async.bulk.reduce.s2g.f16(ptr addrspace(1) %dst, ptr addrspace(3) %src, i32 %size, i32 4, i32 2, i64 %ch, i1 0)
  ret void
}

define void @invalid_scope(ptr addrspace(1) %dst, ptr addrspace(3) %src, i32 %size, i64 %ch) {
  ; CHECK: immarg value 4 for arg 4 out of range [0,4)
  call void @llvm.nvvm.cp.async.bulk.reduce.s2g.u32(ptr addrspace(1) %dst, ptr addrspace(3) %src, i32 %size, i32 3, i32 4, i64 %ch, i1 0)
  ret void
}
