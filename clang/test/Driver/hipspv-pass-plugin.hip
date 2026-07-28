// UNSUPPORTED: system-windows

// RUN: touch %t.dummy.o
// RUN: %clang -### --no-default-config -o /dev/null --target=spirv64-unknown-chipstar \
// RUN:   %t.dummy.o --hip-path=%S/Inputs/hipspv \
// RUN: 2>&1 | FileCheck %s --check-prefixes=ALL,FROM-HIP-PATH

// RUN: %clang -### --no-default-config -o /dev/null --target=spirv64-unknown-chipstar \
// RUN:   %t.dummy.o --hipspv-pass-plugin=%S/Inputs/pass-plugin.so \
// RUN: 2>&1 | FileCheck %s --check-prefixes=ALL,FROM-OPTION

// RUN: not %clang -### --no-default-config -o /dev/null --target=spirv64-unknown-chipstar \
// RUN:   %t.dummy.o --hipspv-pass-plugin=foo.so \
// RUN: 2>&1 | FileCheck %s --check-prefixes=ALL,FROM-OPTION-INVALID

// RUN: %clang -### --no-default-config -o /dev/null --target=spirv64-unknown-chipstar \
// RUN:   %t.dummy.o 2>&1 | FileCheck %s --check-prefixes=ALL,NO-PLUGIN

// FROM-HIP-PATH: {{".*opt"}} {{".*.bc"}} "-load-pass-plugin"
// FROM-HIP-PATH-SAME: {{".*/Inputs/hipspv/lib/libLLVMHipSpvPasses.so"}}
// FROM-OPTION: {{".*opt"}} {{".*.bc"}} "-load-pass-plugin"
// FROM-OPTION-SAME: {{".*/Inputs/pass-plugin.so"}}
// FROM-OPTION-INVALID: error: no such file or directory: 'foo.so'
// NO-PLUGIN-NOT: {{".*opt"}} {{".*.bc"}} "-load-pass-plugin"
// NO-PLUGIN-NOT: {{".*/Inputs/hipspv/lib/libLLVMHipSpvPasses.so"}}
// ALL: {{".*llvm-spirv[^ ]*"}}
