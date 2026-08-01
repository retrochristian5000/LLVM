// RUN: mlir-opt %s --lower-sparse-iteration-to-scf -verify-diagnostics

#DenseCompressed = #sparse_tensor.encoding<{
  map = (d0, d1) -> (d0 : dense, d1 : compressed)
}>

func.func @unsupported_dense_compressed_space(%sp: tensor<?x?xf64, #DenseCompressed>) {
  // expected-error@below {{cannot lower collapsed iteration space with compressed level after the first level}}
  %0 = sparse_tensor.extract_iteration_space %sp lvls = 0 to 2
      : tensor<?x?xf64, #DenseCompressed> -> !sparse_tensor.iter_space<#DenseCompressed, lvls = 0 to 2>
  sparse_tensor.iterate %it in %0 at(%i, %j)
      : !sparse_tensor.iter_space<#DenseCompressed, lvls = 0 to 2> {
  }
  return
}
