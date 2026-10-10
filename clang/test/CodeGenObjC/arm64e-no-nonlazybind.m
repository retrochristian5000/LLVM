// Ensure Objective-C runtime calls also use authenticated stubs on arm64e.
// RUN: %clang_cc1 -triple arm64e-apple-ios -fptrauth-calls -fobjc-arc -emit-llvm %s -o - | FileCheck %s --check-prefix=AUTH
// RUN: %clang_cc1 -triple arm64-apple-ios -fobjc-arc -emit-llvm %s -o - | FileCheck %s --check-prefix=PLAIN

__attribute__((objc_root_class))
@interface Root
- (void)consume:(id)obj;
@end

void dispatch(Root *receiver, id value) {
  [receiver consume:value];
}

// AUTH: declare {{.*}} @objc_msgSend(
// AUTH-NOT: nonlazybind
// PLAIN: declare {{.*}} @objc_msgSend(
// PLAIN: nonlazybind
