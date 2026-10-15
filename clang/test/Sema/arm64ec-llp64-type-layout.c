// RUN: %clang_cc1 -triple x86_64-windows-msvc -std=c11 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple aarch64-windows-msvc -std=c11 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple arm64ec-windows-msvc -std=c11 -fsyntax-only -verify %s
// expected-no-diagnostics
//
// Keep the ARM64EC data model and C record layout compatible with Windows
// x64.  ARM64EC uses ARM instructions, not the Unix AArch64 LP64 data model.

_Static_assert(sizeof(short) == 2, "short must be 16-bit");
_Static_assert(sizeof(int) == 4, "int must be 32-bit");
_Static_assert(sizeof(long) == 4, "Windows LLP64 long must be 32-bit");
_Static_assert(sizeof(long long) == 8, "long long must be 64-bit");
_Static_assert(sizeof(void *) == 8, "pointer must be 64-bit");
_Static_assert(sizeof(long double) == 8, "MS long double must be 64-bit");
_Static_assert(sizeof(__WCHAR_TYPE__) == 2, "Windows wchar_t must be 16-bit");
_Static_assert(__builtin_types_compatible_p(__builtin_va_list, char *),
               "Windows va_list must be char *");

struct TypeBridge {
  char tag;
  void *pointer;
  long count;
  __WCHAR_TYPE__ wide;
  long double number;
};
_Static_assert(__builtin_offsetof(struct TypeBridge, pointer) == 8, "pointer offset");
_Static_assert(__builtin_offsetof(struct TypeBridge, count) == 16, "long offset");
_Static_assert(__builtin_offsetof(struct TypeBridge, wide) == 20, "wchar offset");
_Static_assert(__builtin_offsetof(struct TypeBridge, number) == 24, "long double offset");
_Static_assert(sizeof(struct TypeBridge) == 32, "ABI struct size");
_Static_assert(_Alignof(struct TypeBridge) == 8, "ABI struct alignment");

#pragma pack(push, 1)
struct PackedBridge {
  char tag;
  void *pointer;
};
#pragma pack(pop)
_Static_assert(sizeof(struct PackedBridge) == 9, "packed struct size");
_Static_assert(_Alignof(struct PackedBridge) == 1, "packed struct alignment");

// Clang's AArch64-specific wchar-size macro must describe the same type
// as its canonical target-independent type-width macro.
#ifdef __ARM_SIZEOF_WCHAR_T
_Static_assert(__ARM_SIZEOF_WCHAR_T == sizeof(__WCHAR_TYPE__),
               "ACLE wchar size must reflect the actual ABI");
#endif
