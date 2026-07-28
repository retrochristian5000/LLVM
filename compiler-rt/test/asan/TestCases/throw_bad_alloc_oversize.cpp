// Throwing operator new must throw std::bad_alloc on allocation failure
// (here triggered by an oversize request) rather than aborting. Opt-in via
// allocator_may_return_null=1.

// RUN: %clangxx_asan -O0 %s -o %t
// RUN: %env_asan_opts=allocator_may_return_null=1 %run %t 2>&1 | FileCheck %s

// Windows asan can't throw bad_alloc; see
// sanitizer_common/sanitizer_new_handler.h.
// UNSUPPORTED: target={{.*windows.*}}
// REQUIRES: stable-runtime

#include <cstdio>
#include <cxxabi.h>
#include <new>
#include <typeinfo>

static const size_t kHugeSize =
#if __LP64__ || defined(_WIN64)
    (1ULL << 40) + 1;
#else
    (3UL << 30) + 1;
#endif

int main() {
  // DIAGNOSTIC (DO NOT MERGE): print test-side typeinfo pointer for
  // std::bad_alloc so CI logs let us compare it to the runtime-side pointer.
  fprintf(stderr, "DIAG-TEST-BADALLOC-TINFO: %p\n",
          (const void *)&typeid(std::bad_alloc));
  fflush(stderr);
  bool caught = false;
  try {
    char *p = new char[kHugeSize];
    fprintf(stderr, "FAIL: allocation unexpectedly returned %p\n", p);
  } catch (const std::bad_alloc &e) {
    fprintf(stderr, "DIAG-TEST-CAUGHT-BADALLOC: caught=%p thrown=%p\n",
            (const void *)&typeid(std::bad_alloc), (const void *)&typeid(e));
    fflush(stderr);
    caught = true;
  } catch (...) {
    // DIAGNOSTIC (DO NOT MERGE): distinguish typeinfo-identity failure
    // from wrong-type-thrown from exception-died-before-user-code.
    const std::type_info *ti = abi::__cxa_current_exception_type();
    fprintf(stderr,
            "DIAG-TEST-CAUGHT-OTHER: expected_badalloc=%p got_ti=%p "
            "got_name=%s\n",
            (const void *)&typeid(std::bad_alloc), (const void *)ti,
            ti ? ti->name() : "<null>");
    fflush(stderr);
    caught = true;
  }
  if (caught)
    fprintf(stderr, "caught bad_alloc\n");
  // CHECK: caught bad_alloc
  return 0;
}
