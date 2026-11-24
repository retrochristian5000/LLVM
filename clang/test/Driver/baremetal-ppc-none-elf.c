// RUN: %clang -### %s --target=powerpc-none-elf -fuse-ld=lld -nostdlib 2>&1 \
// RUN:   | FileCheck %s

// CHECK: "-cc1" "-triple" "powerpc-unknown-none-elf"
// CHECK-SAME: "-nostdsysteminc"
// CHECK-NOT: {{.*}}gcc{{(.exe)?}}
// CHECK: {{.*}}ld.lld{{(.exe)?}}
// CHECK-SAME: "-Bstatic"

// RUN: %clang -### %s --target=powerpc-elf -fuse-ld=lld -nostdlib 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PPC32
// RUN: %clang -### %s --target=powerpc-unknown-none-unknown-elf -fuse-ld=lld -nostdlib 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PPC32
// RUN: %clang -### %s --target=powerpc-unknown-none-none-elf -fuse-ld=lld -nostdlib 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PPC32
// RUN: %clang -### %s --target=powerpc-elf -m64 -fuse-ld=lld -nostdlib 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PPC64
// RUN: %clang -### %s --target=powerpc64-elf -m32 -fuse-ld=lld -nostdlib 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PPC32
// RUN: %clang -### %s --target=powerpc64le-unknown-none-unknown-elf -fuse-ld=lld -nostdlib 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PPC64LE

// PPC32: "-cc1" "-triple" "powerpc-{{[^"]+}}"
// PPC32-SAME: "-nostdsysteminc"
// PPC32-NOT: {{.*}}gcc{{(.exe)?}}
// PPC32: {{.*}}ld.lld{{(.exe)?}}"
// PPC32-SAME: "-Bstatic" "-m" "elf32ppc"
// PPC64: "-cc1" "-triple" "powerpc64-{{[^"]+}}"
// PPC64-SAME: "-nostdsysteminc"
// PPC64-NOT: {{.*}}gcc{{(.exe)?}}
// PPC64: {{.*}}ld.lld{{(.exe)?}}"
// PPC64-SAME: "-Bstatic" "-m" "elf64ppc"
// PPC64LE: "-cc1" "-triple" "powerpc64le-{{[^"]+}}"
// PPC64LE-SAME: "-nostdsysteminc"
// PPC64LE-NOT: {{.*}}gcc{{(.exe)?}}
// PPC64LE: {{.*}}ld.lld{{(.exe)?}}"
// PPC64LE-SAME: "-Bstatic" "-m" "elf64lppc"

int main(void) { return 0; }
