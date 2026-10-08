// Check that Darwin Clang sends linker concurrency options to Mach-O LLD,
// not to LLVM's internal -mllvm command-line parser.
//
// RUN: %clang -target arm64-apple-macos13.0 -fuse-ld=lld -flto=thin -flto-jobs=2 -### %s -o %t 2>&1 | FileCheck %s --check-prefix=LLD-LTO
// LLD-LTO: "--threads=2"
// LLD-LTO-NOT: "-threads=2"
//
// RUN: %clang -target arm64-apple-macos13.0 -fuse-ld= -flto=thin -flto-jobs=2 -### %s -o %t 2>&1 | FileCheck %s --check-prefix=APPLE-LTO
// APPLE-LTO: "-mllvm" "-threads=2"
//
// RUN: %clang -target arm64-apple-macos13.0 -fuse-ld=lld -threads -### %s -o %t 2>&1 | FileCheck %s --check-prefix=BARE
// RUN: %clang -target arm64-apple-macos13.0 -fuse-ld=lld --threads -### %s -o %t 2>&1 | FileCheck %s --check-prefix=BARE
// BARE: "--threads"
//
// RUN: %clang -target arm64-apple-macos13.0 -fuse-ld=lld -threads=3 -### %s -o %t 2>&1 | FileCheck %s --check-prefix=VALUE
// RUN: %clang -target arm64-apple-macos13.0 -fuse-ld=lld --threads=3 -### %s -o %t 2>&1 | FileCheck %s --check-prefix=VALUE
// VALUE: "--threads=3"
//
// RUN: %clang -target arm64-apple-macos13.0 -fuse-ld=lld -flto=thin -flto-jobs=2 -threads=3 -### %s -o %t 2>&1 | FileCheck %s --check-prefix=OVERRIDE
// OVERRIDE: "--threads=2"
// OVERRIDE: "--threads=3"
//
// RUN: not %clang -target arm64-apple-macos13.0 -fuse-ld= -threads=2 -### %s -o %t 2>&1 | FileCheck %s --check-prefix=APPLE-DISALLOW
// APPLE-DISALLOW: unsupported option '-threads=2'
//
// RUN: %clang -target arm64-apple-macos13.0 -fuse-ld=lld -c %s -o %t.o
int threads_driver_probe(void) { return 0; }
