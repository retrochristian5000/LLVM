# REQUIRES: x86
# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t.o
# RUN: ld.lld -shared -z nodelete %t.o -o %t.so
# RUN: llvm-readelf --dynamic %t.so | FileCheck %s --check-prefix=NODELETE
# RUN: ld.lld -shared %t.o -o %t.normal.so
# RUN: llvm-readelf --dynamic %t.normal.so | FileCheck %s --check-prefix=NORMAL

# NODELETE: (FLAGS_1){{.*}}NODELETE
# NORMAL: Dynamic section
# NORMAL-NOT: NODELETE

.globl f
.type f,@function
f:
  ret
