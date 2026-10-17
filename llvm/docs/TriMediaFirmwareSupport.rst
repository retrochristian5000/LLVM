TriMedia firmware object-file groundwork
======================================

TriMedia is recognized by LLVM's Triple parser as two 32-bit architectures:
``trimedia`` (little-endian) and ``trimediaeb`` (big-endian).
These are object-file identities, not independent processor revisions.

The ELF reader now recognizes existing ``EM_TRIMEDIA`` (163) as
``elf32-littletrimedia`` or ``elf32-bigtrimedia``, using EI_DATA to
select the byte order. 64-bit ELF is not classified as TriMedia.

This change supports inspection of recovered 32-bit firmware objects.
It does NOT add a TriMedia Clang driver, target machine, instruction
encoder/decoder, assembler, linker relocation definitions, calling
convention, or firmware runtime. Compiling C firmware with
``clang --target=trimedia-unknown-none`` is NOT supported yet.

NXP PNX15xx/952x Series Data Book, revision 4.0 (3 December 2007),
documents the TM3260 as a 32-bit VLIW CPU, 128 32-bit registers,
and software-switchable little/big endianness:

https://www.nxp.com/docs/en/data-sheet/PNX15XX_PNX952X_SER_N.pdf

The TM3260/PNX15xx description must not be silently assigned to the
AT&T 2Wire 2701HG-B chipset without independent evidence about that
specific board.

Next: recover TriMedia ELF relocations, instruction packet encodings,
C ABI, stack alignment and calling convention from original tools and
documentation; implement MC and backend incrementally with QEMU
cross-checks. Modem LEDs remain QEMU device output, not compiler data.
