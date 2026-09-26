// Check that ld gets "-lto_library".

// RUN: mkdir -p %t/bin
// RUN: mkdir -p %t/lib
// RUN: touch %t/lib/libLTO.dylib
// RUN: %clang -fuse-ld= --target=x86_64-apple-darwin10 -### %s \
// RUN:   -ccc-install-dir %t/bin -mlinker-version=133 2> %t.log
// RUN: FileCheck -check-prefix=LINK_LTOLIB_PATH %s -input-file %t.log
//
// LINK_LTOLIB_PATH: {{ld(.exe)?"}}
// LINK_LTOLIB_PATH: "-lto_library"

// Do not advertise a libLTO path that does not exist. Standalone LLVM
// installations may intentionally omit libLTO.dylib, and passing the missing
// path makes ld64 warn on every link.
// RUN: %clang -fuse-ld= --target=x86_64-apple-darwin10 -### %s \
// RUN:   -ccc-install-dir %S/dummytestdir -mlinker-version=133 2> %t.log
// RUN: FileCheck -check-prefix=NO_LINK_LTOLIB_PATH %s -input-file %t.log
//
// NO_LINK_LTOLIB_PATH: {{ld(.exe)?"}}
// NO_LINK_LTOLIB_PATH-NOT: "-lto_library"


// Check that -object_lto_path is passed correctly to ld64
// RUN: %clang -fuse-ld= --target=x86_64-apple-darwin10 %s -flto=full \
// RUN:     -mlinker-version=116 -### 2>&1 \
// RUN:     | FileCheck -check-prefix=FULL_LTO_OBJECT_PATH %s
// FULL_LTO_OBJECT_PATH: {{ld(.exe)?"}}
// FULL_LTO_OBJECT_PATH-SAME: "-object_path_lto"
// FULL_LTO_OBJECT_PATH-SAME: {{cc\-[a-zA-Z0-9_]+.o}}"
// RUN: %clang -fuse-ld= --target=x86_64-apple-darwin10 %s -flto=thin \
// RUN:     -mlinker-version=116 -### 2>&1 \
// RUN:     | FileCheck -check-prefix=THIN_LTO_OBJECT_PATH %s
// THIN_LTO_OBJECT_PATH: {{ld(.exe)?"}}
// THIN_LTO_OBJECT_PATH-SAME: "-object_path_lto"
// THIN_LTO_OBJECT_PATH-SAME: {{thinlto\-[a-zA-Z0-9_]+}}


// Check that we pass through -fglobal-isel flags to libLTO.
// RUN: %clang --target=arm64-apple-darwin %s -flto -fglobal-isel -### 2>&1 | \
// RUN:   FileCheck --check-prefix=GISEL %s
// GISEL: {{ld(.exe)?"}}
// GISEL: "-mllvm" "-global-isel"
// GISEL: "-mllvm" "-global-isel-abort=0"


// Check that we disable atexit()-based global destructor lowering when
// compiling/linking for kernel/kext/freestanding.
// RUN: %clang --target=arm64-apple-darwin %s -flto -fapple-kext -### 2>&1 | \
// RUN:   FileCheck --check-prefix=KEXT %s
// KEXT: {{ld(.exe)?"}}
// KEXT: "-mllvm" "-disable-atexit-based-global-dtor-lowering"

// Check that we select a filename for -fstack-usage.
// RUN: %clang --target=arm64-apple-darwin %s -flto -fstack-usage -o foo \
// RUN:   -### 2>&1 | FileCheck --check-prefix=STACKUSAGE %s
// STACKUSAGE: "-mllvm" "-stack-usage-file=foo.su"
