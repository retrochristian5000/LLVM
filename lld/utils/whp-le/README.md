# WHP experimental LE writer

Standalone Python prototype located alongside the WHP LLVM fork's LLD utilities.
It translates a restricted collection of freestanding i386 COFF object files
into MZ-wrapped Windows 386 dynamic LE VxD images.

Build an object using the WHP LLVM Clang executable:

    clang --target=i386-pc-windows-msvc -ffreestanding -fno-builtin -fno-stack-protector -c example.c -o example.obj
    python3 lld/utils/whp-le-link.py --name WHPTEST --ddb WHPTEST_DDB -o WHPTEST.VXD example.obj

An existing DDB symbol is REQUIRED; this tool does not implement VMM services
or construct a Device Descriptor Block. The leading i386 symbol underscore is
resolved as needed. The name becomes the LE resident module name, and the
DDB becomes export ordinal 1.

Supported now: i386 COFF, multiple .obj inputs, text/rdata/data/bss sections,
ordinary internal DIR32 and REL32 fixups, and zero-filled LE pages.
Unsupported features fail: COMDAT, imports, CRT, archives, BigObj, exceptions,
unhandled relocation kinds, and cross-page relocation sources.

Not an integrated native LLD backend; not proven Windows 95 RTM loadable.
Do not ship drivers based on header inspection alone. Verify format and then
test VMM initialization and DDB control callback in a disposable VM.

References:
https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2305.HTM
https://www.geoffchappell.com/notes/windows/archive/vxdldr.htm
