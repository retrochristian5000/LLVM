// RUN: %clang -target powerpc-none-elf -mcpu=604 -msoft-float \
// RUN:   -ffreestanding -fno-pic -fno-pie -O0 -c %s -o %t.o
// RUN: llvm-readelf -hW -rW %t.o | FileCheck %s --check-prefix=OBJ \
// RUN:   --implicit-check-not=R_PPC_GOT --implicit-check-not=R_PPC_SDA \
// RUN:   --implicit-check-not=R_PPC_EMB_SDA

// OpenBIOS' 32-bit PowerPC entry code does not initialize the SDA/TOC base
// registers (r2/r13). The freestanding static ELF path must use absolute
// HA/LO relocations rather than small-data or GOT addressing. The firmware
// also implements __moddi3, which Clang uses for signed 64-bit remainder.
// This is a PPC32 ABI guard, not a PPC64 ELFv1/ELFv2 test.

// OBJ: Class: ELF32
// OBJ: Data: 2's complement, big endian
// OBJ: Machine: PowerPC
// OBJ: R_PPC_ADDR16_HA
// OBJ: R_PPC_ADDR16_LO
// OBJ: R_PPC_REL24{{.*}}__moddi3

volatile unsigned int openbios_word;

unsigned int *openbios_address(void) {
  return (unsigned int *)&openbios_word;
}

long long openbios_moddi(long long a, long long b) {
  return a % b;
}
