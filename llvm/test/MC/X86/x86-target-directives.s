# RUN: not llvm-mc -triple i386 -filetype asm -o - %s 2>&1 | FileCheck %s

	.arch i8086
	.arch i186
	.arch i286
	.arch i386
	.code42

# CHECK: error: x86 .arch does not select an instruction-set baseline
# CHECK: error: x86 .arch does not select an instruction-set baseline
# CHECK: error: x86 .arch does not select an instruction-set baseline
# CHECK: error: x86 .arch does not select an instruction-set baseline
# CHECK: unknown directive .code42
# CHECK-NOT: unknown directive

