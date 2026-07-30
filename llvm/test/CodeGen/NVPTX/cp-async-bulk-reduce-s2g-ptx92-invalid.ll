; RUN: not --crash llc < %s -mtriple=nvptx64 -mcpu=sm_90 -mattr=+ptx92 2>&1 | FileCheck %s

target triple = "nvptx64-nvidia-cuda"

declare void @llvm.nvvm.cp.async.bulk.reduce.s2g.u32(ptr addrspace(1), ptr addrspace(3), i32, i32 immarg, i32 immarg, i64, i1 immarg)

define void @cp_async_bulk_reduce_s2g_ptx92_invalid_scope(ptr addrspace(3) %src, ptr addrspace(1) %dst, i32 %size, i64 %ch) {
; CHECK: LLVM ERROR: cp.reduce.async.bulk sem.scope requires PTX ISA 9.3 or higher
  tail call void @llvm.nvvm.cp.async.bulk.reduce.s2g.u32(ptr addrspace(1) %dst, ptr addrspace(3) %src, i32 %size, i32 3, i32 1, i64 %ch, i1 0)
  ret void
}
