// REQUIRES: shell
// Check GNU-compatible -B lookup and linker library search ordering.
//
// RUN: mkdir -p %t/first %t/second %t/explicit %t/sysroot/prefix
// RUN: touch %t/cross-libwhp.a %t/first/libwhp.a %t/second/libwhp.a %t/sysroot/prefix/libwhp.a
// RUN: %clang --target=x86_64-unknown-linux-gnu -B%t/cross- -B%t/first -print-file-name=libwhp.a | FileCheck %s --check-prefix=PREFIX -DROOT=%t
// RUN: %clang --target=x86_64-unknown-linux-gnu -B%t/first -B%t/cross- -print-file-name=libwhp.a | FileCheck %s --check-prefix=DIRECTORY -DROOT=%t
// RUN: %clang --target=x86_64-unknown-linux-gnu --sysroot=%t/sysroot -B=/prefix -print-file-name=libwhp.a | FileCheck %s --check-prefix=SYSROOT -DROOT=%t
// RUN: %clang --target=x86_64-unknown-linux-gnu -### -nostdlib -B%t/first -B%t/cross- -B%t/second -L%t/explicit %s -lwhp 2>&1 | FileCheck %s --check-prefix=LINK -DROOT=%t
//
// PREFIX: [[ROOT]]/cross-libwhp.a
// DIRECTORY: [[ROOT]]/first/libwhp.a
// SYSROOT: [[ROOT]]/sysroot/prefix/libwhp.a
// LINK: "-L[[ROOT]]/explicit" "-L[[ROOT]]/first" "-L[[ROOT]]/second"
// LINK-NOT: "-L[[ROOT]]/cross-"
// LINK-SAME: "-lwhp"
