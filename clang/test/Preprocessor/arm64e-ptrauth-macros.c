// RUN: %clang -target arm64e-apple-macosx13.0 -fsyntax-only -x c %s
// RUN: %clang -target arm64-apple-macosx13.0 -fsyntax-only -x c %s
// RUN: %clang -target arm64e-apple-macosx13.0 -fsyntax-only -x c++ %s
// RUN: %clang -target arm64-apple-macosx13.0 -fsyntax-only -x c++ %s

// The arm64e PAC ABI requires both indirect-call authentication and Clang's
// pointer-authentication intrinsic interface. __PTRAUTH__ is the documented
// feature macro, distinct from the legacy __PTRAUTH_INTRINSICS__ spelling.
// Neither may leak into plain arm64 code through target-family detection.

#if defined(__arm64e__)
#ifndef __PTRAUTH__
#error arm64e lost the documented __PTRAUTH__ feature macro
#endif
#ifndef __PTRAUTH_INTRINSICS__
#error arm64e lost the legacy pointer-auth intrinsic macro
#endif
#if !__has_feature(ptrauth_calls)
#error arm64e lost indirect-call authentication
#endif
#if !__has_feature(ptrauth_intrinsics)
#error arm64e lost pointer-authentication intrinsic support
#endif
#else
#ifdef __PTRAUTH__
#error normal arm64 must not claim the arm64e pointer-authentication ABI
#endif
#ifdef __PTRAUTH_INTRINSICS__
#error normal arm64 must not claim the arm64e intrinsic ABI
#endif
#endif

int whp_arm64e_ptrauth_macro_probe(void) { return 0; }
