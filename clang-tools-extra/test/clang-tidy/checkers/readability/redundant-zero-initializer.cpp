// RUN: %check_clang_tidy -std=c++14-or-later %s readability-redundant-zero-initializer %t

char a[12] = {0};
// CHECK-MESSAGES: :[[@LINE-1]]:14: warning: redundant zero initializer; replace with empty braces [readability-redundant-zero-initializer]
// CHECK-FIXES: char a[12] = {};

int b[5] = {0};
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: redundant zero initializer; replace with empty braces
// CHECK-FIXES: int b[5] = {};

double d[3] = {0};
// CHECK-MESSAGES: :[[@LINE-1]]:15: warning: redundant zero initializer; replace with empty braces
// CHECK-FIXES: double d[3] = {};

void *p[4] = {0};
// CHECK-MESSAGES: :[[@LINE-1]]:14: warning: redundant zero initializer; replace with empty braces
// CHECK-FIXES: void *p[4] = {};

char one[1] = {0};
// CHECK-MESSAGES: :[[@LINE-1]]:15: warning: redundant zero initializer; replace with empty braces
// CHECK-FIXES: char one[1] = {};

const char cq[8] = {0};
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: redundant zero initializer; replace with empty braces
// CHECK-FIXES: const char cq[8] = {};

int spaced[2] = { 0 };
// CHECK-MESSAGES: :[[@LINE-1]]:17: warning: redundant zero initializer; replace with empty braces
// CHECK-FIXES: int spaced[2] = {};

int trailingComma[3] = {0,};
// CHECK-MESSAGES: :[[@LINE-1]]:24: warning: redundant zero initializer; replace with empty braces
// CHECK-FIXES: int trailingComma[3] = {};

int paren[2] = {(0)};
// CHECK-MESSAGES: :[[@LINE-1]]:16: warning: redundant zero initializer; replace with empty braces
// CHECK-FIXES: int paren[2] = {};

double nestedParen[2] = {((0))};
// CHECK-MESSAGES: :[[@LINE-1]]:25: warning: redundant zero initializer; replace with empty braces
// CHECK-FIXES: double nestedParen[2] = {};

struct S {
  char buf[4] = {0};
  // CHECK-MESSAGES: :[[@LINE-1]]:17: warning: redundant zero initializer; replace with empty braces
  // CHECK-FIXES: char buf[4] = {};
};

void localVariables() {
  int local[4] = {0};
  // CHECK-MESSAGES: :[[@LINE-1]]:18: warning: redundant zero initializer; replace with empty braces
  // CHECK-FIXES: int local[4] = {};
  static int staticLocal[2] = {0};
  // CHECK-MESSAGES: :[[@LINE-1]]:31: warning: redundant zero initializer; replace with empty braces
  // CHECK-FIXES: static int staticLocal[2] = {};
}

using Arr = int[2];
void use(const int (&)[2]);
void arrayPrvalue() {
  use(Arr{0});
  // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: redundant zero initializer; replace with empty braces
  // CHECK-FIXES: use(Arr{});
}

char emptyBraces[12] = {};
int nonZero[3] = {1};
int multiZero[3] = {0, 0};
int mixed[4] = {0, 5};
int noInit[3];

// Multi-dimensional arrays without inner braces: the single `0` initializes a
// subobject through brace elision, so there is no written single-element `{0}`
// list to rewrite.
int twoD[2][3] = {0};
int oneByOne[1][1] = {0};
int rowVector[1][3] = {0};
int columnVector[3][1] = {0};

// The array bound is deduced from the initializer, `{}` would change the size.
char deduced[] = {0};

char nullChar[4] = {'\0'};
double zeroDouble[3] = {0.0};
void *nullPtr[4] = {nullptr};

int scalar = {0};

struct P { int x; int y; };
P pod = {0};

// Array of a class type with a default member initializer: `{0}` and `{}` are
// not equivalent, so it must not be rewritten.
struct WithDefault { int v = 7; };
WithDefault wd[2] = {0};

// Template instantiations share the pattern's written braces. Rewriting `{0}`
// in the pattern would break the `templateFn<X>` instantiation, whose element
// type is not default-constructible, so neither the pattern nor any of its
// instantiations may be flagged.
struct X {
  X(int);
  X() = delete;
};

template <class T>
void templateFn() {
  T arr[1] = {0};
}

void instantiate() {
  templateFn<int>();
  templateFn<X>();
}
