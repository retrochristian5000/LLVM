// RUN: %clang --target=arm64-apple-macosx11.0 -S -emit-llvm -O0 %s -o - | FileCheck %s --check-prefix=CPU --implicit-check-not=+sve --implicit-check-not=+mec
// RUN: %clang --target=arm64-apple-macosx11.0 -march=armv9.2-a -S -emit-llvm -O0 %s -o - | FileCheck %s --check-prefix=ARCH
//
// Keep the Armv9.2-A architecture version distinct from its optional
// extension defaults.  The architecture-wide -march baseline enables its
// existing compatibility defaults; CPU-specific target attributes must not
// silently re-enable SVE/SVE2 or MEC on Apple M4/A18.
//
// CPU-DAG: "target-features"="{{[^"]*}}+v9.2a{{[^"]*}}"
// CPU-DAG: "target-features"="{{[^"]*}}+v9.2a{{[^"]*}}"
// ARCH: "target-features"="{{[^"]*}}+sve2{{[^"]*}}"

__attribute__((target("cpu=apple-m4")))
int whp_m4_v92_no_sve(void) { return 1; }

__attribute__((target("cpu=apple-a18")))
int whp_a18_v92_no_sve(void) { return 2; }

int whp_generic_v92(void) { return 3; }
