// REQUIRES: x86-registered-target
// RUN: %clang --target=x86_64-unknown-linux-gnu -march=x86-64 -mneeded -c %s -o %t
// RUN: llvm-readelf --notes %t | FileCheck %s --check-prefix=BASE
// RUN: %clang --target=x86_64-unknown-linux-gnu -march=x86-64-v2 -mneeded -c %s -o %t
// RUN: llvm-readelf --notes %t | FileCheck %s --check-prefix=V2
// RUN: %clang --target=x86_64-unknown-linux-gnu -march=x86-64-v3 -mneeded -c %s -o %t
// RUN: llvm-readelf --notes %t | FileCheck %s --check-prefix=V3
// RUN: %clang --target=x86_64-unknown-linux-gnu -march=x86-64-v4 -mneeded -fcf-protection=full -c %s -o %t
// RUN: llvm-readelf --notes %t | FileCheck %s --check-prefixes=CET,V4
// RUN: %clang --target=x86_64-unknown-linux-gnu -march=x86-64 -mlzcnt -mneeded -c %s -o %t
// RUN: llvm-readelf --notes %t | FileCheck %s --check-prefix=LZCNT
// RUN: %clang --target=i686-unknown-linux-gnu -march=i686 -mneeded -c %s -o %t
// RUN: llvm-readelf --notes %t | FileCheck %s --check-prefix=BASE
// RUN: %clang --target=x86_64-unknown-linux-gnu -march=x86-64-v3 -mneeded -mno-needed -c %s -o %t
// RUN: llvm-readelf --notes %t | FileCheck %s --allow-empty --check-prefix=OFF
// RUN: %clang --target=x86_64-unknown-linux-gnu -march=x86-64-v2 -mno-needed -mneeded -c %s -o %t
// RUN: llvm-readelf --notes %t | FileCheck %s --check-prefix=V2
// RUN: not %clang --target=aarch64-unknown-linux-gnu -mneeded -c %s -o %t 2>&1 | FileCheck %s --check-prefix=BAD
// RUN: not %clang --target=x86_64-apple-macosx13.0 -mneeded -c %s -o %t 2>&1 | FileCheck %s --check-prefix=BAD
// RUN: %clang --target=x86_64-unknown-linux-gnu -march=x86-64-v2 -mneeded -emit-llvm -c %s -o %t.a.bc
// RUN: %clang --target=x86_64-unknown-linux-gnu -march=x86-64 -mlzcnt -mneeded -DFN=bar -emit-llvm -c %s -o %t.b.bc
// RUN: llvm-link %t.a.bc %t.b.bc -o %t.bc
// RUN: llc -filetype=obj %t.bc -o %t
// RUN: llvm-readelf --notes %t | FileCheck %s --check-prefix=V3
//
// BASE: x86 ISA needed: x86-64-baseline{{$}}
// V2: x86 ISA needed: x86-64-baseline, x86-64-v2{{$}}
// V3: x86 ISA needed: x86-64-baseline, x86-64-v2, x86-64-v3{{$}}
// CET: x86 feature: IBT, SHSTK
// V4: x86 ISA needed: x86-64-baseline, x86-64-v2, x86-64-v3, x86-64-v4{{$}}
// LZCNT: x86 ISA needed: x86-64-baseline, x86-64-v3{{$}}
// OFF-NOT: x86 ISA needed:
// BAD: error: unsupported option '-mneeded' for target

#ifndef FN
#define FN foo
#endif
void FN(void) {}
