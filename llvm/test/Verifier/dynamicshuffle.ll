; RUN: not opt -passes=verify -S < %s 2>&1 | FileCheck %s

; CHECK: Mask must be a vector of integers.
define <4 x i32> @mask_not_integer(<4 x i32> %v1, <4 x i32> %v2, <4 x float> %mask) {
  %res = call <4 x i32> @llvm.dynamicshuffle.v4i32.v4i32.v4f32(<4 x i32> %v1, <4 x i32> %v2, <4 x float> %mask)
  ret <4 x i32> %res
}

; CHECK: Mask and return type must have the same number of elements.
define <4 x i32> @mask_wrong_count(<4 x i32> %v1, <4 x i32> %v2, <8 x i8> %mask) {
  %res = call <4 x i32> @llvm.dynamicshuffle.v4i32.v4i32.v8i8(<4 x i32> %v1, <4 x i32> %v2, <8 x i8> %mask)
  ret <4 x i32> %res
}

; CHECK: Return type and input vectors must have the same element type.
define <4 x float> @wrong_element_type(<4 x i32> %v1, <4 x i32> %v2, <4 x i8> %mask) {
  %res = call <4 x float> @llvm.dynamicshuffle.v4f32.v4i32.v4i8(<4 x i32> %v1, <4 x i32> %v2, <4 x i8> %mask)
  ret <4 x float> %res
}

; CHECK: Return type and input vectors must both be fixed or both be scalable vectors.
define <vscale x 4 x i32> @mixed_scalable(<4 x i32> %v1, <4 x i32> %v2, <vscale x 4 x i8> %mask) {
  %res = call <vscale x 4 x i32> @llvm.dynamicshuffle.nxv4i32.v4i32.nxv4i8(<4 x i32> %v1, <4 x i32> %v2, <vscale x 4 x i8> %mask)
  ret <vscale x 4 x i32> %res
}
