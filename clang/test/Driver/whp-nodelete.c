// RUN: %clang --help | FileCheck %s --check-prefix=HELP
// RUN: %clang -target x86_64-unknown-linux-gnu -shared -nostdlib -### %s -Wl,-z,nodelete 2>&1 | FileCheck %s --check-prefix=ELF
// RUN: %clang -target aarch64-unknown-linux-gnu -shared -nostdlib -### %s -z nodelete 2>&1 | FileCheck %s --check-prefix=ELF
// RUN: %clang -target x86_64-unknown-linux-gnu -shared -nostdlib -### %s -Wl,-z,nodelete,-z,now 2>&1 | FileCheck %s --check-prefix=MIXED
// RUN: %clang -target x86_64-unknown-linux-gnu -shared -nostdlib -### %s -Xlinker -z -Xlinker nodelete 2>&1 | FileCheck %s --check-prefix=ELF
// RUN: not %clang -target arm64-apple-macosx14.0 -dynamiclib -nostdlib -### %s -Wl,-z,nodelete 2>&1 | FileCheck %s --check-prefix=MACHO
// RUN: not %clang -target x86_64-apple-macosx14.0 -dynamiclib -nostdlib -### %s -z nodelete 2>&1 | FileCheck %s --check-prefix=MACHO

// HELP: -Wl,-z,nodelete
// HELP-SAME: Mark an ELF shared object as not unloadable at runtime
// ELF: "-z" "nodelete"
// MIXED: "-z" "nodelete" "-z" "now"
// MACHO: error: unsupported option '-z nodelete' for target '{{.*}}apple-macosx{{.*}}'
