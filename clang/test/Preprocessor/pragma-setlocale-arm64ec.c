// RUN: %clang_cc1 -triple arm64ec-pc-windows-msvc -fms-extensions -Wno-unknown-pragmas -fsyntax-only -verify=EC %s
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fms-extensions -Wunknown-pragmas -fsyntax-only -verify=X64 %s
// RUN: %clang_cc1 -triple aarch64-pc-windows-msvc -fms-extensions -Wunknown-pragmas -fsyntax-only -verify=ARM64 %s
//
// MSVC's setlocale pragma influences the encoding of wide literals at
// compile time. An ARM64EC object must not silently ignore that request,
// even with -Wunknown-pragmas disabled. Do not alter existing diagnostics
// for other Windows ABIs until matching locale conversion is implemented.

#pragma setlocale("dutch") // EC-error {{#pragma setlocale is unsupported for ARM64EC}} // X64-warning {{unknown pragma ignored}} // ARM64-warning {{unknown pragma ignored}}
#pragma setlocale("C") // EC-error {{#pragma setlocale is unsupported for ARM64EC}} // X64-warning {{unknown pragma ignored}} // ARM64-warning {{unknown pragma ignored}}
#pragma setlocale("") // EC-error {{#pragma setlocale is unsupported for ARM64EC}} // X64-warning {{unknown pragma ignored}} // ARM64-warning {{unknown pragma ignored}}

// __pragma is Microsoft syntax and must not bypass the ABI guard.
__pragma(setlocale("French_France.1252")) // EC-error {{#pragma setlocale is unsupported for ARM64EC}} // X64-warning {{unknown pragma ignored}} // ARM64-warning {{unknown pragma ignored}}

const __WCHAR_TYPE__ *wide = L"\u00e9";
