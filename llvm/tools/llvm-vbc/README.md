# llvm-vbc: experimental classic Visual Basic source-to-LLVM IR

This is not a complete Microsoft Visual Basic compiler. It is an isolated,
fail-closed, classic VB6/VBA-style standard-module subset producing textual
LLVM IR. Unsupported syntax is rejected rather than silently ignored.

Supported: case-insensitive ASCII identifiers; apostrophe and full-line REM
comments; mandatory module-level Option Explicit; exactly one [Public]
Function Name() As Long ... End Function; Dim name As Long; assignment to
declared local variables and to the function name; 32-bit signed Long
literals (including -2147483648); unary +/-; parentheses; +, -, and *;
Exit Function only as the final effective statement. Locals and the result
are zero-initialized.

Generated functions are C-callable ABI symbols vb_<lowercase name>, not
Visual Basic native runtime ABI. Function Main() generates vb_main().
Arithmetic overflow triggers llvm.trap (not a faithful Visual Basic
trappable runtime exception). The caller supplies target, libraries,
and final linker configuration.

NOT SUPPORTED: VB Integer (16-bit), Variant, Boolean, String, arrays,
floating point, If/For/Do, calls/parameters, error handling, forms, COM,
properties, events, VB project files, VB p-code, VB.NET, 16-bit targets,
Windows 95 executable startup, or DOS device-driver ABI.

Example (example.bas):

    Option Explicit
    Public Function Main() As Long
        Dim Count As Long
        Count = 20
        Main = Count + 22
    End Function

    llvm-vbc example.bas -o example.ll
    llvm-as example.ll -o example.bc
    llc -filetype=obj example.bc -o example.o

A compatible external C ABI caller can call vb_main() and receive 42.
The frontend does not emit native VB6 projects or link the VB runtime.

Reference: Microsoft's classic VB/VBA documentation for Option Explicit,
Function ... End Function, and signed 32-bit Long. This program is a
language frontend and must remain separate from DOS/Win16/Win9x runtimes.
