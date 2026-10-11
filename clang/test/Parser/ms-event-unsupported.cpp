// Microsoft event-source/receiver attributes must never be silently
// ignored: they generate native dispatch or COM connection-point code.
// Actual __event/__hook/__unhook/__raise support remains unimplemented.
// RUN: %clang_cc1 -triple i386-pc-windows-msvc -std=c++11 -fms-extensions -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -std=c++11 -fms-extensions -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple aarch64-pc-windows-msvc -std=c++11 -fms-extensions -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple i386-pc-windows-msvc -std=c++2c -fms-extensions -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -std=c++2c -fms-extensions -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple aarch64-pc-windows-msvc -std=c++2c -fms-extensions -fsyntax-only -verify %s

[event_source(native)] // expected-error {{Microsoft event attribute 'event_source' is unsupported (requires event code generation)}}
struct NativeSource {};

[event_receiver(native)] // expected-error {{Microsoft event attribute 'event_receiver' is unsupported (requires event code generation)}}
struct NativeReceiver {};

[event_source(com)] // expected-error {{Microsoft event attribute 'event_source' is unsupported (requires event code generation)}}
struct ComSource {};

[event_receiver(com, layout_dependent=true)] // expected-error {{Microsoft event attribute 'event_receiver' is unsupported (requires event code generation)}}
struct ComReceiver {};

// Unknown MS attributes continue to have the previous compatibility
// behavior; rejecting arbitrary Microsoft bracket attributes is not needed.
[water_unknown_attribute(anything)]
struct Other {};
