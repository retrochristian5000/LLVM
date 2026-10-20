// RUN: %clang --driver-mode=g++ -std=c++17 -fsyntax-only \
// RUN:   -Werror=unknown-warning-option -Wformat-overflow=2 \
// RUN:   -Xclang -verify %s
// RUN: %clang --driver-mode=g++ -std=c++17 -fsyntax-only \
// RUN:   -Werror=unknown-warning-option -Werror=format-overflow=2 \
// RUN:   -Xclang -verify %s
// RUN: %clang --driver-mode=g++ -std=c++17 -fsyntax-only \
// RUN:   -Werror=unknown-warning-option -Wno-format-overflow=2 \
// RUN:   -Xclang -verify %s
// RUN: %clang --driver-mode=g++ -std=c++17 -fsyntax-only \
// RUN:   -Werror=unknown-warning-option -Wno-error=format-overflow=2 \
// RUN:   -Xclang -verify %s

// QEMU uses GCC's -Wformat-overflow=2 spelling in meson.build.
// Clang accepts the GCC spelling and enables its own format-overflow
// diagnostic group; the GCC level-2 analysis is not reproduced here.
// expected-no-diagnostics
int whp_format_overflow_warning_compat() { return 0; }
