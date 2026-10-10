# REQUIRES: x86
#
# When linking directly to a PE DLL (MinGW mode), an export at the exact
# end of .text belongs to the following section, not .text.  Misclassifying
# that address as code creates a jump thunk for a data variable.
#
# RUN: split-file %s %t.dir
# RUN: llvm-mc -triple=x86_64-windows-gnu -filetype=obj %t.dir/library.s -o %t.lib.obj
# RUN: lld-link /dll /noentry /out:%t.lib.dll /implib:%t.unused.lib /export:code_export /export:data_export,DATA %t.lib.obj
# RUN: llvm-objdump -h %t.lib.dll | FileCheck --check-prefix=SECTIONS %s
# RUN: llvm-objdump -p %t.lib.dll | FileCheck --check-prefix=EXPORTS %s
# RUN: llvm-mc -triple=x86_64-windows-gnu -filetype=obj %t.dir/main.s -o %t.main.obj
# RUN: lld-link -lldmingw /entry:mainCRTStartup /subsystem:console /opt:noref /out:%t.main.exe %t.main.obj %t.lib.dll /verbose 2>&1 | FileCheck --check-prefix=AUTOIMPORT %s
# RUN: llvm-readobj --coff-imports %t.main.exe | FileCheck --check-prefix=IMPORTS %s

# SECTIONS: .text 00001000
# SECTIONS: .rdata
#
# EXPORTS: Export Table:
# EXPORTS: Ordinal      RVA  Name
# EXPORTS-NEXT: {{ *}}1   0x1000  code_export
# EXPORTS-NEXT: {{ *}}2   0x2000  data_export
#
# AUTOIMPORT: Automatically importing data_export from {{.*}}.lib.dll
#
# IMPORTS: Import {
# IMPORTS: Name: {{.*}}.lib.dll
# IMPORTS: Symbol: code_export
# IMPORTS: Symbol: data_export

#--- library.s
.text
.globl code_export
code_export:
  ret
# Deliberately end the .text virtual section exactly at the first byte
# of .rdata after 4 KiB section alignment.
.space 4095

.section .rdata,"dr"
.globl data_export
data_export:
  .long 0x12345678

#--- main.s
.text
.globl mainCRTStartup
mainCRTStartup:
  call code_export
  movq .refptr.data_export(%rip), %rax
  movl (%rax), %eax
  ret

.section .rdata$.refptr.data_export,"dr",discard,.refptr.data_export
.globl .refptr.data_export
.refptr.data_export:
  .quad data_export
