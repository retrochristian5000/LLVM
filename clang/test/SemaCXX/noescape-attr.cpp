// RUN: %clang_cc1 -triple arm64e-apple-macosx13.0 -std=c++17 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple x86_64-apple-macosx13.0 -std=c++17 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple aarch64-unknown-linux-gnu -std=c++17 -fsyntax-only -verify %s

#if !__has_feature(attribute_noescape_nonpointer)
#error "Missing record-parameter noescape support"
#endif

template<typename T>
void test1(T __attribute__((noescape)) arr, int size);

void test2(int __attribute__((noescape)) arr, int size); // expected-warning {{'noescape' attribute only applies to a pointer, reference, class, struct, or union}}

struct Record { int *p; };
class Class { int *p; };
union Union { int *p; long n; };
void test3(Record __attribute__((noescape)));
void test4(Class __attribute__((noescape)));
void test5(Union __attribute__((noescape)));
void test6(int & __attribute__((noescape)));
void test7(int * __attribute__((noescape)));
