// RUN: %clang -### %s --target=powerpc-none-elf -fuse-ld=lld -nostdlib 2>&1 \
// RUN:   | FileCheck %s

// CHECK: "-cc1" "-triple" "powerpc-unknown-none-elf"
// CHECK-SAME: "-nostdsysteminc"
// CHECK-NOT: {{.*}}gcc{{(.exe)?}}
// CHECK: {{.*}}ld.lld{{(.exe)?}}
// CHECK-SAME: "-Bstatic"

int main(void) { return 0; }
