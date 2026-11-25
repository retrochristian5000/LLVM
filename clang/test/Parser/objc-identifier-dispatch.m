// RUN: %clang_cc1 -fsyntax-only -verify -x c %s
// RUN: %clang_cc1 -fsyntax-only -verify -x c++ %s
// RUN: %clang_cc1 -fsyntax-only -verify -x objective-c %s
// RUN: %clang_cc1 -fsyntax-only -verify -x objective-c++ %s

#ifdef __OBJC__
__attribute__((objc_root_class))
@interface Base
@property(class, readonly) int count;
+ (int)value:(int)x;
@end

@interface Derived : Base
+ (int)test;
@end

@implementation Derived
+ (int)test {
  int a = Base.count;
  int b = super.count;
  int c = Base value:1]; // expected-error {{missing '[' at start of message send expression}}
  int d = super value:2]; // expected-error {{missing '[' at start of message send expression}}
  return a + b + c + d;
}
@end
#else
// expected-no-diagnostics
struct Counter { int count; };
int read_count(struct Counter super) {
  return super.count;
}
#endif
