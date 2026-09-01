// RUN: %clang -Werror -fsyntax-only -fdefer-pop %s
// RUN: %clang -Werror -fsyntax-only -fno-defer-pop %s
// RUN: %clang -### -fsyntax-only -fdefer-pop %s 2>&1 | FileCheck %s --check-prefix=DRIVER --implicit-check-not=-fdefer-pop
// RUN: %clang -### -fsyntax-only -fno-defer-pop %s 2>&1 | FileCheck %s --check-prefix=DRIVER --implicit-check-not=-fno-defer-pop
//
// DRIVER: "-cc1"

int whp_defer_pop;
