// Check to ensure we have a .tmp for unoptimized bitcode with -fno-gpu-rdc.
// RUN: %clang -### -ccc-print-bindings --target=x86_64-linux-gnu -nogpulib \
// RUN:   -save-temps -fno-gpu-rdc -nogpuinc --offload-arch=gfx906 -c %s 2>&1 | \
// RUN:   FileCheck -check-prefixes=NORDC-BINDINGS %s
//      NORDC-BINDINGS: "amdgcn-amd-amdhsa" - "clang", inputs: ["[[INPUT:.+]]"], output: "[[HIPI:.+]]"
// NORDC-BINDINGS-NEXT: "amdgcn-amd-amdhsa" - "clang", inputs: ["[[HIPI]]"], output: "[[TMP_BC:.+]]"
// NORDC-BINDINGS-NEXT: "amdgcn-amd-amdhsa" - "clang", inputs: ["[[TMP_BC]]"], output: "[[BC:.+]]"
// NORDC-BINDINGS-NEXT: "amdgcn-amd-amdhsa" - "Offload::Packager", inputs: ["[[BC]]"], output: "[[OUTPUT:.+]]"
// NORDC-BINDINGS-NEXT: "amdgcn-amd-amdhsa" - "Offload::Linker", inputs: ["[[OUTPUT]]"], output: "[[FATBIN:.+]]"

// Check to ensure we have a .tmp for unoptimized bitcode with -fgpu-rdc.
// RUN: %clang -### -ccc-print-bindings --target=x86_64-linux-gnu -nogpulib \
// RUN:   -save-temps -fgpu-rdc -nogpuinc --offload-arch=gfx906 -c %s 2>&1 | \
// RUN:   FileCheck -check-prefixes=RDC-BINDINGS %s
//      RDC-BINDINGS: # "amdgcn-amd-amdhsa" - "clang", inputs: ["[[INPUT:.+]]"], output: "[[HIPI:.+]]"
// RDC-BINDINGS-NEXT: # "amdgcn-amd-amdhsa" - "clang", inputs: ["[[HIPI]]"], output: "[[TMP_BC:.+]]"
// RDC-BINDINGS-NEXT: # "amdgcn-amd-amdhsa" - "clang", inputs: ["[[TMP_BC]]"], output: "[[BC:.+]]"
// RDC-BINDINGS-NEXT: # "x86_64-unknown-linux-gnu" - "Offload::Packager", inputs: ["[[BC]]"], output: "[[OUTPUT:.+]]"
