// UNSUPPORTED: system-windows
// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: sed -e "s|DIR|%/t|g" %t/cdb.json.template > %t/cdb.json

// RUN: clang-scan-deps -compilation-database %t/cdb.json \
// RUN:   -format experimental-full -j 1 -o %t/deps.json
// RUN: FileCheck %s --input-file %t/scan.log

// CHECK: [{{[0-9]+\.[0-9]+}}] [[#PID:]] [[#TID:]]: starting scanning command:{{.*}}tu.c
// CHECK: [{{[0-9]+\.[0-9]+}}] {{.*}}: pcm_write: {{.*}}.pcm
// CHECK: [{{[0-9]+\.[0-9]+}}] {{.*}}: finished scanning command:{{.*}}tu.c
//--- cdb.json.template
[{
  "directory": "DIR",
  "command": "clang -fsyntax-only DIR/tu.c -fmodules -fimplicit-module-maps -fmodules-cache-path=DIR/cache -fbuild-session-timestamp=1 -fmodules-validate-once-per-build-session -fdepscan-log-path=DIR/scan.log",
  "file": "DIR/tu.c"
}]

//--- module.modulemap
module A { header "A.h" }
//--- A.h
void A_func(void);
//--- tu.c
#include "A.h"
void foo(void) { A_func(); }
