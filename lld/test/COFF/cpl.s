# REQUIRES: x86
#
# Control Panel applets are PE DLLs with a .cpl extension. The extension
# must survive the link and the exported name must be exactly CPlApplet.
# This is a DLL ABI test, not a special new executable format.
#
# RUN: yaml2obj %p/Inputs/export.yaml -o %t.x64.obj
# RUN: lld-link /dll /noentry /opt:ref /out:%t.cpl %t.x64.obj /export:CPlApplet=exportfn1
# RUN: llvm-objdump -p %t.cpl | FileCheck --check-prefix=X64 %s
# RUN: llvm-objdump -p %t.cpl | FileCheck --check-prefix=DLL-FLAGS %s
#
# A .cpl used as MSVC-style linker input is as invalid as a .dll input.
# Check case-insensitivity of the extension.
# RUN: cp %t.cpl %t.CPL
# RUN: not lld-link /dll /noentry /out:%t.invalid.dll %t.CPL 2>&1 | FileCheck --check-prefix=INPUT %s
#
# On i686, __stdcall decorates the COFF symbol; Control Panel still expects
# the undecorated public export name. Use an explicit linker export alias.
# RUN: llvm-mc -triple=i686-windows-msvc -filetype=obj %s -o %t.x86.obj
# RUN: lld-link /dll /noentry /safeseh:no /out:%t.x86.cpl %t.x86.obj "/export:CPlApplet=_CPlApplet@16"
# RUN: llvm-objdump -p %t.x86.cpl | FileCheck --check-prefix=X86 %s
#
# The applet export must be retained by the LTO and /opt:ref link paths.
# RUN: llvm-as -o %t.lto.obj %p/Inputs/export.ll
# RUN: lld-link /dll /noentry /opt:ref /out:%t.lto.cpl %t.lto.obj /export:CPlApplet=exportfn1
# RUN: llvm-objdump -p %t.lto.cpl | FileCheck --check-prefix=LTO %s
#
# X64: Characteristics
# X64: DLL
# X64: AddressOfEntryPoint{{ *}}00000000
# X64: Export Table:
# X64: DLL name: cpl.s.tmp.cpl
# X64: Ordinal      RVA  Name
# X64: CPlApplet
#
# DLL-FLAGS: DLL
# DLL-FLAGS: AddressOfEntryPoint{{ *}}00000000
#
# X86: Characteristics
# X86: DLL
# X86: AddressOfEntryPoint{{ *}}00000000
# X86: Export Table:
# X86: DLL name: cpl.s.tmp.x86.cpl
# X86: Ordinal      RVA  Name
# X86: CPlApplet
# X86-NOT: _CPlApplet@16
#
# LTO: Characteristics
# LTO: DLL
# LTO: Export Table:
# LTO: DLL name: cpl.s.tmp.lto.cpl
# LTO: CPlApplet
#
# INPUT: bad file type. Did you specify a DLL instead of an import library?

.text
.globl _CPlApplet@16
_CPlApplet@16:
  ret
