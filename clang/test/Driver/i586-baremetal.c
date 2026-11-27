// REQUIRES: x86-registered-target
// UNSUPPORTED: system-windows

// Keep the Pentium baseline without an explicit -march, including code16.
// RUN: %clang --target=i586-none-elf -m32 -ffreestanding -mregparm=3 -fno-pic -fno-pie -### -c %s 2>&1 | FileCheck %s --check-prefixes=FLAT,CPU
// RUN: %clang --target=i586-none-elf -m16 -ffreestanding -mregparm=3 -fno-pic -fno-pie -### -c %s 2>&1 | FileCheck %s --check-prefixes=REAL,CPU
// FLAT: "-cc1" "-triple" "i586-unknown-none-elf"
// REAL: "-cc1" "-triple" "i586-unknown-none-code16"
// CPU-SAME: "-target-cpu" "i586"

// An explicit processor selection takes precedence over the triple.
// RUN: %clang --target=i586-none-elf -m16 -march=pentium-mmx -ffreestanding -### -c %s 2>&1 | FileCheck %s --check-prefix=MMX
// MMX: "-cc1" "-triple" "i586-unknown-none-code16"
// MMX-SAME: "-target-cpu" "pentium-mmx"

// The freestanding 32-bit target uses the existing i386 ELF linker ABI.
// RUN: %clang --target=i586-none-elf -m32 -nostdlib -ffreestanding -fuse-ld=lld -### %s 2>&1 | FileCheck %s --check-prefix=LINK
// LINK: ld.lld{{.*}}" "-Bstatic" "-m" "elf_i386"
// LINK-NOT: crt0.o
// LINK-NOT: libclang_rt
// LINK-NOT: "-lc"

// The old compiler-rt layout uses the canonical i386 suffix for i586.
// RUN: rm -rf %t
// RUN: mkdir -p %t/resource/lib/baremetal %t/sysroot/lib
// RUN: touch %t/resource/lib/baremetal/libclang_rt.builtins-i386.a
// RUN: %clang --target=i586-none-elf --sysroot=%t/sysroot -resource-dir=%t/resource --rtlib=compiler-rt -print-libgcc-file-name | FileCheck %s --check-prefix=BUILTINS
// BUILTINS: {{[/\\]}}resource{{[/\\]}}lib{{[/\\]}}baremetal{{[/\\]}}libclang_rt.builtins-i386.a

unsigned probe(unsigned a, unsigned b, unsigned c) { return a + b + c; }
