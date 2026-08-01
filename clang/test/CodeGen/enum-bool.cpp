// RUN: %clang_cc1 -triple x86_64-unknown-unknown -emit-llvm -o - %s | FileCheck %s

namespace dr2338 {
namespace A {
enum E { Zero, One };
E a(int x) { return static_cast<E>(x); }
// CHECK-LABEL: define{{.*}} i32 @_ZN6dr23381A1aEi
// CHECK: ret i32 %0

E b(int x) { return (E)x; }
// CHECK-LABEL: define{{.*}} i32 @_ZN6dr23381A1bEi
// CHECK: ret i32 %0

} // namespace A
namespace B {
enum E : bool { Zero, One };
E a(int x) { return static_cast<E>(x); }
// CHECK-LABEL: define{{.*}} zeroext i1 @_ZN6dr23381B1aEi
// CHECK: ret i1 %tobool

E b(int x) { return (E)x; }
// CHECK-LABEL: define{{.*}} zeroext i1 @_ZN6dr23381B1bEi
// CHECK: ret i1 %tobool

E c(float x) { return static_cast<E>(x); }
// CHECK-LABEL: define{{.*}} zeroext i1 @_ZN6dr23381B1cEf
// CHECK: ret i1 %tobool

E d(float x) { return (E)x; }
// CHECK-LABEL: define{{.*}} zeroext i1 @_ZN6dr23381B1dEf
// CHECK: ret i1 %tobool

E e(double x) { return static_cast<E>(x); }
// CHECK-LABEL: define{{.*}} zeroext i1 @_ZN6dr23381B1eEd
// CHECK: ret i1 %tobool

E f(double x) { return (E)x; }
// CHECK-LABEL: define{{.*}} zeroext i1 @_ZN6dr23381B1fEd
// CHECK: ret i1 %tobool
} // namespace B
namespace C {
enum class E { Zero, One };
E a(int x) { return static_cast<E>(x); }
// CHECK-LABEL: define{{.*}} i32 @_ZN6dr23381C1aEi
// CHECK: ret i32 %0

E b(int x) { return (E)x; }
// CHECK-LABEL: define{{.*}} i32 @_ZN6dr23381C1bEi
// CHECK: ret i32 %0

} // namespace C
namespace D {
enum class E : bool { Zero, One };
E a(int x) { return static_cast<E>(x); }
// CHECK-LABEL: define{{.*}} zeroext i1 @_ZN6dr23381D1aEi
// CHECK: ret i1 %tobool

E b(int x) { return (E)x; }

// CHECK-LABEL: define{{.*}} zeroext i1 @_ZN6dr23381D1bEi
// CHECK: ret i1 %tobool

E c(float x) { return static_cast<E>(x); }
// CHECK-LABEL: define{{.*}} zeroext i1 @_ZN6dr23381D1cEf
// CHECK: ret i1 %tobool

E d(float x) { return (E)x; }
// CHECK-LABEL: define{{.*}} zeroext i1 @_ZN6dr23381D1dEf
// CHECK: ret i1 %tobool

E e(double x) { return static_cast<E>(x); }
// CHECK-LABEL: define{{.*}} zeroext i1 @_ZN6dr23381D1eEd
// CHECK: ret i1 %tobool

E f(double x) { return (E)x; }
// CHECK-LABEL: define{{.*}} zeroext i1 @_ZN6dr23381D1fEd
// CHECK: ret i1 %tobool

} // namespace D
} // namespace dr2338
