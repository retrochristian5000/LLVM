// Verify the bare-metal baseline without requiring a sysroot or runtime.
// RUN: %clang -target i386-none-elf -m32 -### -c %s 2>&1 | FileCheck %s --check-prefix=I386
// RUN: %clang -target i386-none-elf -m16 -### -c %s 2>&1 | FileCheck %s --check-prefix=I386
// RUN: %clang -target i586-none-elf -m32 -### -c %s 2>&1 | FileCheck %s --check-prefix=I586
// RUN: %clang -target i586-none-elf -m16 -### -c %s 2>&1 | FileCheck %s --check-prefix=I586
// RUN: %clang -target i486-none-elf -### -c %s 2>&1 | FileCheck %s --check-prefix=I486
// RUN: %clang -target i686-none-elf -### -c %s 2>&1 | FileCheck %s --check-prefix=I686
// RUN: %clang -target i586-none-elf -m16 -march=pentium-mmx -### -c %s 2>&1 | FileCheck %s --check-prefix=MMX
// RUN: %clang -target i586-none-elf -march=i386 -### -c %s 2>&1 | FileCheck %s --check-prefix=I386
// RUN: %clang -target i386-unknown-linux-gnu -### -c %s 2>&1 | FileCheck %s --check-prefix=LINUX
// I386: "-target-cpu" "i386"
// I486: "-target-cpu" "i486"
// I586: "-target-cpu" "i586"
// I686: "-target-cpu" "i686"
// MMX: "-target-cpu" "pentium-mmx"
// LINUX: "-target-cpu" "pentium4"
int firmware_probe(int value) { return value + 1; }
