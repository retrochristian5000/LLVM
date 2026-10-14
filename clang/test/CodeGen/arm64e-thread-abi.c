// REQUIRES: aarch64-registered-target
//
// Darwin arm64e must authenticate the thread-local variable resolver and sign
// pthread start function pointers.  Keep ordinary arm64 as a negative control.
// Atomic acquire-release ordering must not drift between the two ABIs.
// These are compile-only ABI tests; neither host executes the generated code.
//
// RUN: %clang -target arm64e-apple-macosx13.0 -O0 -S %s -o - | FileCheck %s --check-prefix=EASM
// RUN: %clang -target arm64-apple-macosx13.0  -O0 -S %s -o - | FileCheck %s --check-prefix=AASM
// RUN: %clang -target arm64e-apple-macosx13.0 -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=EIR
// RUN: %clang -target arm64-apple-macosx13.0  -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=AIR

typedef void *(*ThreadStart)(void *);
struct opaque_pthread;
typedef struct opaque_pthread *pthread_t;
extern int pthread_create(pthread_t *, const void *, ThreadStart, void *);

_Thread_local int tls_counter;
_Atomic(unsigned long) shared_counter;

static void *thread_entry(void *payload) {
  ++tls_counter;
  return payload;
}

// EIR: @thread_entry.ptrauth = private constant
// EIR: @tls_counter = thread_local global
// EIR-LABEL: define{{.*}} i32 @spawn(
// EIR: call i32 @pthread_create({{.*}}ptr noundef @thread_entry.ptrauth,
// AIR-NOT: @thread_entry.ptrauth
// AIR: @tls_counter = thread_local global
// AIR-LABEL: define{{.*}} i32 @spawn(
// AIR: call i32 @pthread_create({{.*}}ptr noundef @thread_entry,

// EASM-LABEL: _spawn:
// EASM: paciza x{{[0-9]+}}
// EASM: bl _pthread_create
// AASM-LABEL: _spawn:
// AASM-NOT: paciza
// AASM: bl _pthread_create
int spawn(pthread_t *thread, void *payload) {
  return pthread_create(thread, 0, thread_entry, payload);
}

// EIR-LABEL: define internal ptr @thread_entry(
// EIR: @llvm.threadlocal.address.p0(
// AIR-LABEL: define internal ptr @thread_entry(
// AIR: @llvm.threadlocal.address.p0(

// EASM-LABEL: _thread_entry:
// EASM: {{_tls_counter@TLVPPAGE}}
// EASM: blraaz x{{[0-9]+}}
// AASM-LABEL: _thread_entry:
// AASM: {{_tls_counter@TLVPPAGE}}
// AASM: blr x{{[0-9]+}}

// EASM-LABEL: _read_tls:
// EASM: {{_tls_counter@TLVPPAGE}}
// EASM: blraaz x{{[0-9]+}}
// AASM-LABEL: _read_tls:
// AASM: {{_tls_counter@TLVPPAGE}}
// AASM: blr x{{[0-9]+}}
int read_tls(void) { return tls_counter; }

// EIR-LABEL: define{{.*}} i64 @bump_shared(
// EIR: atomicrmw add ptr @shared_counter, i64 {{.*}} acq_rel
// AIR-LABEL: define{{.*}} i64 @bump_shared(
// AIR: atomicrmw add ptr @shared_counter, i64 {{.*}} acq_rel
unsigned long bump_shared(void) {
  return __c11_atomic_fetch_add(&shared_counter, 1, __ATOMIC_ACQ_REL);
}

// A start callback may be published between threads with release/acquire
// ordering. Atomic memory operations must not strip the function signature;
// the eventual call must authenticate the loaded callback.
// EIR-LABEL: define void @publish_start(
// EIR: store atomic i64 {{.*}}, ptr @published_start release
// AIR-LABEL: define void @publish_start(
// AIR: store atomic i64 {{.*}}, ptr @published_start release
// EASM-LABEL: _publish_start:
// EASM: paciza x{{[0-9]+}}
// EASM: stlr x{{[0-9]+}}, [x{{[0-9]+}}]
// AASM-LABEL: _publish_start:
// AASM-NOT: paciza
// AASM: stlr x{{[0-9]+}}, [x{{[0-9]+}}]
_Atomic(ThreadStart) published_start;
void publish_start(void) {
  __c11_atomic_store(&published_start, thread_entry, __ATOMIC_RELEASE);
}

// EIR-LABEL: define{{.*}} ptr @invoke_start(
// EIR: load atomic i64, ptr @published_start acquire
// EIR: call ptr %{{.*}}(ptr noundef %{{.*}}) [ "ptrauth"(i32 0, i64 0) ]
// AIR-LABEL: define{{.*}} ptr @invoke_start(
// AIR: load atomic i64, ptr @published_start acquire
// AIR: call ptr %{{.*}}(ptr noundef %{{.*}})
// AIR-NOT: "ptrauth"
// AIR: ret ptr
// EASM-LABEL: _invoke_start:
// EASM: {{ldar|ldapr}} x{{[0-9]+}}, [x{{[0-9]+}}]
// EASM: blraaz x{{[0-9]+}}
// AASM-LABEL: _invoke_start:
// AASM: {{ldar|ldapr}} x{{[0-9]+}}, [x{{[0-9]+}}]
// AASM: blr x{{[0-9]+}}
void *invoke_start(void *payload) {
  ThreadStart fn = __c11_atomic_load(&published_start, __ATOMIC_ACQUIRE);
  return fn(payload);
}
