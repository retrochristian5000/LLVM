// REQUIRES: amdgpu-registered-target
// REQUIRES: lld

// Test HIP non-RDC linker wrapper behavior with new offload driver.
// The linker wrapper should output .hipfb files directly without using -r option.

// An externally visible variable so static libraries extract.
__attribute__((visibility("protected"), used)) int x;

// Create device binaries and package them
// RUN: %clang -cc1 %s -triple amdgpu-amd-amdhsa -emit-llvm-bc -o %t.amdgpu.bc
// RUN: llvm-offload-binary -o %t.out \
// RUN:   --image=file=%t.amdgpu.bc,kind=hip,triple=amdgpu9.4-amd-amdhsa,arch=gfx9-4-generic:xnack+ \
// RUN:   --image=file=%t.amdgpu.bc,kind=hip,triple=amdgpu12.00-amd-amdhsa,arch=gfx1200
// RUN: llvm-offload-binary -o %t.single.out \
// RUN:   --image=file=%t.amdgpu.bc,kind=hip,triple=amdgpu12.00-amd-amdhsa,arch=gfx1200

// Test that linker wrapper outputs .hipfb file without -r option for HIP non-RDC
// The linker wrapper is called directly with the packaged device binary (not embedded in host object)
// Note: When called directly (not through the driver), the linker wrapper processes architectures
// from the packaged binary. The test verifies it can process at least one architecture correctly.
// RUN: %if system-windows %{ \
// RUN:   clang-linker-wrapper --host-triple=x86_64-unknown-linux-gnu --wrapper-verbose --device-linker=amdgpu-amd-amdhsa=-v --device-compiler=-v --emit-fatbin-only --linker-path=/usr/bin/ld %t.out -o %t.hipfb 2>&1 | FileCheck %s --check-prefix=CMD-WIN \
// RUN: %} %else %{ \
// RUN:   clang-linker-wrapper --host-triple=x86_64-unknown-linux-gnu --wrapper-verbose --device-linker=amdgpu-amd-amdhsa=-v --device-compiler=-v --emit-fatbin-only --linker-path=/usr/bin/ld %t.out -o %t.hipfb 2>&1 | FileCheck %s --check-prefix=CMD-LINUX \
// RUN: %}

// On Linux, ':' is preserved in file names
// CMD-LINUX-DAG: clang{{.*}} -o {{.*}}hipfb.amdgpu9.4.gfx9-4-generic:xnack+{{.*}}.img
// CMD-LINUX-DAG: clang{{.*}} -o {{.*}}hipfb.amdgpu12.00.gfx1200{{.*}}.img
// CMD-LINUX-DAG: ld.lld{{.*}} -o {{.*}}hipfb.amdgpu9.4.gfx9-4-generic:xnack+{{.*}}.img
// CMD-LINUX-DAG: ld.lld{{.*}} -o {{.*}}hipfb.amdgpu12.00.gfx1200{{.*}}.img

// On Windows, ':' is replaced with '@' in file names
// CMD-WIN-DAG: clang{{.*}} -o {{.*}}hipfb.amdgpu9.4.gfx9-4-generic@xnack+{{.*}}.img
// CMD-WIN-DAG: clang{{.*}} -o {{.*}}hipfb.amdgpu12.00.gfx1200{{.*}}.img
// CMD-WIN-DAG: ld.lld{{.*}} -o {{.*}}hipfb.amdgpu9.4.gfx9-4-generic@xnack+{{.*}}.img
// CMD-WIN-DAG: ld.lld{{.*}} -o {{.*}}hipfb.amdgpu12.00.gfx1200{{.*}}.img

// Verify the fat binary was created
// RUN: test -f %t.hipfb

// List code objects in the fat binary
// RUN: clang-offload-bundler -type=o -input=%t.hipfb -list | FileCheck %s --check-prefix=HIP-FATBIN-LIST

// HIP-FATBIN-LIST-DAG: hip-amdgpu9.4-amd-amdhsa--gfx9-4-generic:xnack+
// HIP-FATBIN-LIST-DAG: hip-amdgpu12.00-amd-amdhsa--gfx1200
// HIP-FATBIN-LIST-DAG: host-x86_64-unknown-linux-gnu

// Extract code objects for both architectures from the fat binary
// Use '-' instead of ':' in file names to avoid issues on Windows
// RUN: clang-offload-bundler -type=o -targets=hip-amdgpu9.4-amd-amdhsa--gfx9-4-generic:xnack+,hip-amdgpu12.00-amd-amdhsa--gfx1200 \
// RUN:   -output=%t.gfx9-4-generic-xnack+.co -output=%t.gfx1200.co -input=%t.hipfb -unbundle

// Verify extracted code objects exist and are not empty
// RUN: test -f %t.gfx9-4-generic-xnack+.co
// RUN: test -s %t.gfx9-4-generic-xnack+.co
// RUN: test -f %t.gfx1200.co
// RUN: test -s %t.gfx1200.co

// Emit one linked code object without bundling it into a HIP fat binary.
// RUN: clang-linker-wrapper --host-triple=x86_64-unknown-linux-gnu \
// RUN:   --emit-device-images-only --linker-path=/usr/bin/ld %t.single.out \
// RUN:   -o %t.raw.co
// RUN: llvm-readobj --file-headers %t.raw.co | FileCheck %s --check-prefix=RAW

// RAW: Format: elf64-amdgpu

// Emit each linked code object to the filename selected by the driver.
// RUN: clang-linker-wrapper --emit-device-images-only \
// RUN:   --device-image-output=amdgpu9.4-amd-amdhsa,gfx9-4-generic:xnack+ %t.raw-gfx9-4-generic-xnack+.co \
// RUN:   --device-image-output=amdgpu12.00-amd-amdhsa,gfx1200 %t.raw-gfx1200.co \
// RUN:   --linker-path=/usr/bin/ld %t.out
// RUN: test -f %t.raw-gfx1200.co
// RUN: test -f %t.raw-gfx9-4-generic-xnack+.co

// Clang passes the exact output filename for each device image.
// RUN: rm -rf %t.dir && mkdir %t.dir
// RUN: cd %t.dir && %clang -x hip --cuda-device-only \
// RUN:   --no-gpu-bundle-output --offload-arch=gfx900 \
// RUN:   --offload-arch=gfx1200 -nogpuinc -nogpulib %s
// RUN: test -f %t.dir/linker-wrapper-hip-no-rdc-hip-amdgcn-amd-amdhsa-gfx900.out
// RUN: test -f %t.dir/linker-wrapper-hip-no-rdc-hip-amdgcn-amd-amdhsa-gfx1200.out

// The implicit filename keeps its target suffix for one architecture too.
// RUN: rm -rf %t.single.dir && mkdir %t.single.dir
// RUN: cd %t.single.dir && %clang -x hip --cuda-device-only \
// RUN:   --no-gpu-bundle-output --offload-arch=gfx1200 \
// RUN:   -nogpuinc -nogpulib %s
// RUN: test -f %t.single.dir/linker-wrapper-hip-no-rdc-hip-amdgcn-amd-amdhsa-gfx1200.out

// An explicit output name is invalid for multiple device images.
// RUN: not clang-linker-wrapper --emit-device-images-only \
// RUN:   --linker-path=/usr/bin/ld %t.out -o %t.multiple 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MULTIPLE

// MULTIPLE: error: cannot specify -o when emitting multiple device images

// Every device image needs exactly one output mapping.
// RUN: not clang-linker-wrapper --emit-device-images-only \
// RUN:   --device-image-output=amdgpu12.00-amd-amdhsa,gfx1200 %t.raw-gfx1200.co \
// RUN:   --linker-path=/usr/bin/ld %t.out 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-OUTPUT

// MISSING-OUTPUT: error: expected an output file for each linked device image

// RUN: not clang-linker-wrapper --emit-device-images-only \
// RUN:   --device-image-output=amdgpu12.00-amd-amdhsa,gfx1200 %t.raw-gfx1200.co \
// RUN:   --device-image-output=amdgpu12.00-amd-amdhsa,gfx1200 %t.raw-gfx1200-duplicate.co \
// RUN:   --linker-path=/usr/bin/ld %t.single.out 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DUPLICATE-OUTPUT

// DUPLICATE-OUTPUT: error: duplicate output for device image 'amdgpu12.00-amd-amdhsa,gfx1200'

// RUN: not clang-linker-wrapper --emit-device-images-only \
// RUN:   --device-image-output=amdgpu12.00-amd-amdhsa,gfx1200 %t.raw-gfx1200.co \
// RUN:   --linker-path=/usr/bin/ld %t.single.out -o %t.raw.co 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OUTPUT-CONFLICT

// OUTPUT-CONFLICT: error: cannot combine -o with explicit device image outputs

// RUN: not clang-linker-wrapper --emit-fatbin-only \
// RUN:   --emit-device-images-only --linker-path=/usr/bin/ld %t.single.out \
// RUN:   -o %t.invalid 2>&1 | FileCheck %s --check-prefix=CONFLICT

// CONFLICT: error: cannot emit a fat binary and raw device images together
