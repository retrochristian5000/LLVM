# REQUIRES: x86
#
# ActiveX controls are PE DLL images, even when their extension is .ocx.
# Check the DLL bit, undecorated COM exports, TYPELIB resource retention,
# and import-library naming; the type-library bytes here are only a test
# sentinel, not a usable COM type library or a working ActiveX control.
#
# RUN: split-file %s %t.dir
# RUN: llvm-mc -triple=x86_64-windows-msvc -filetype=obj %t.dir/control64.s -o %t.dir/control64.obj
# RUN: llvm-rc -no-preprocess /FO %t.dir/typelib.res -- %t.dir/typelib.rc
# RUN: lld-link /nologo /dll /noentry /machine:x64 /out:%t.dir/control.ocx \
# RUN:   /implib:%t.dir/control.lib /export:DllCanUnloadNow /export:DllGetClassObject \
# RUN:   /export:DllRegisterServer /export:DllUnregisterServer \
# RUN:   %t.dir/control64.obj %t.dir/typelib.res
# RUN: llvm-readobj --file-headers %t.dir/control.ocx | FileCheck %s --check-prefix=DLL
# RUN: llvm-readobj --coff-exports %t.dir/control.ocx | FileCheck %s --check-prefix=EXPORTS
# RUN: llvm-readobj --coff-resources %t.dir/control.ocx | FileCheck %s --check-prefix=RESOURCE
# RUN: llvm-ar t %t.dir/control.lib | FileCheck %s --check-prefix=IMPLIB
#
# DLL: IMAGE_FILE_DLL
# EXPORTS:      Name: DllCanUnloadNow
# EXPORTS:      Name: DllGetClassObject
# EXPORTS:      Name: DllRegisterServer
# EXPORTS:      Name: DllUnregisterServer
# RESOURCE: TYPELIB
# IMPLIB: control.ocx
#
# The import library must reference the OCX image, not a guessed .dll.
# RUN: llvm-mc -triple=x86_64-windows-msvc -filetype=obj %t.dir/client64.s -o %t.dir/client64.obj
# RUN: lld-link /nologo /entry:mainCRTStartup /subsystem:console /out:%t.dir/client-lib.exe \
# RUN:   %t.dir/client64.obj %t.dir/control.lib
# RUN: llvm-readobj --coff-imports %t.dir/client-lib.exe | FileCheck %s --check-prefix=IMPORT
#
# MinGW mode can link directly against a PE image (including an OCX).
# RUN: lld-link /nologo /lldmingw /entry:mainCRTStartup /subsystem:console \
# RUN:   /out:%t.dir/client-direct.exe %t.dir/client64.obj %t.dir/control.ocx
# RUN: llvm-readobj --coff-imports %t.dir/client-direct.exe | FileCheck %s --check-prefix=IMPORT
#
# IMPORT: Name: control.ocx
# IMPORT: Symbol: DllCanUnloadNow
#
# Native MSVC link mode requires the import library, and should explain
# that an OCX is a DLL rather than misdiagnosing an unknown input format.
# RUN: not lld-link /nologo /entry:mainCRTStartup /subsystem:console \
# RUN:   /out:%t.dir/bad.exe %t.dir/client64.obj %t.dir/control.ocx 2>&1 | \
# RUN:   FileCheck %s --check-prefix=BAD-OCX
# BAD-OCX: control.ocx: bad file type. Did you specify a DLL instead of an import library?
#
# The 32-bit COM stdcall ABI uses decorated symbols internally but exports
# undecorated names for GetProcAddress() and regsvr32.
# RUN: llvm-mc -triple=i686-windows-msvc -filetype=obj %t.dir/control32.s -o %t.dir/control32.obj
# RUN: lld-link /nologo /dll /noentry /safeseh:no /machine:x86 /out:%t.dir/control32.ocx \
# RUN:   /export:DllCanUnloadNow=_DllCanUnloadNow@0 \
# RUN:   /export:DllGetClassObject=_DllGetClassObject@12 \
# RUN:   /export:DllRegisterServer=_DllRegisterServer@0 \
# RUN:   /export:DllUnregisterServer=_DllUnregisterServer@0 %t.dir/control32.obj
# RUN: llvm-readobj --coff-exports %t.dir/control32.ocx | FileCheck %s --check-prefix=EXPORTS
#
# /safeseh:no above is only for the assembly-only regression fixture;
# real 32-bit controls should carry the appropriate safe-SEH metadata.
#
#--- control64.s
.text
.globl DllCanUnloadNow
DllCanUnloadNow:
  xorl %eax, %eax
  ret
.globl DllGetClassObject
DllGetClassObject:
  xorl %eax, %eax
  ret
.globl DllRegisterServer
DllRegisterServer:
  xorl %eax, %eax
  ret
.globl DllUnregisterServer
DllUnregisterServer:
  xorl %eax, %eax
  ret
#
#--- control32.s
.text
.globl "_DllCanUnloadNow@0"
"_DllCanUnloadNow@0":
  xorl %eax, %eax
  ret
.globl "_DllGetClassObject@12"
"_DllGetClassObject@12":
  xorl %eax, %eax
  ret $12
.globl "_DllRegisterServer@0"
"_DllRegisterServer@0":
  xorl %eax, %eax
  ret
.globl "_DllUnregisterServer@0"
"_DllUnregisterServer@0":
  xorl %eax, %eax
  ret
#
#--- client64.s
.text
.globl mainCRTStartup
mainCRTStartup:
  call DllCanUnloadNow
  ret
#
#--- typelib.rc
// A custom TYPELIB resource proves resource propagation, not MIDL parsing.
// Embedding an actual compiled .tlb is the control author's responsibility.
1 TYPELIB { 0x534d, 0x5446 }
