// RUN: %clang_cc1 -fsyntax-only -std=c++26 %s

namespace GH170991 {
struct S { int x{}; };

template <typename = void>
void f() {
  constexpr S s;
  constexpr auto [...xs] = s;
}

template void f<void>();
}
