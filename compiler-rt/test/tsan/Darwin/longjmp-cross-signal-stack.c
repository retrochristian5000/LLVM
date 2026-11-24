// RUN: %clang_tsan -O1 -pthread %s -o %t && %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
// RUN: %clang_tsan -O2 -pthread %s -o %t.opt && %run %t.opt 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
//
// A signal handler on a higher-address alternate stack must not retire a
// live checkpoint on the interrupted, lower-address pthread stack.

#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>

static sigjmp_buf outer;
static sigjmp_buf inner;
static uintptr_t outer_address;
static volatile sig_atomic_t handler_ok;

static void on_signal(int signo) {
  volatile char marker = 0;
  if (signo != SIGUSR1 || (uintptr_t)&marker <= outer_address)
    siglongjmp(outer, 2);
  int value = sigsetjmp(inner, 1);
  if (!value)
    siglongjmp(inner, 7);
  if (value == 7)
    handler_ok = 1;
  siglongjmp(outer, 11);
}

struct stacks {
  void *alternate;
  size_t size;
};

static void *run(void *arg) {
  const struct stacks *stacks = arg;
  stack_t alternate = {0};
  alternate.ss_sp = stacks->alternate;
  alternate.ss_size = stacks->size;
  if (sigaltstack(&alternate, NULL))
    return (void *)1;
  struct sigaction action = {0};
  action.sa_handler = on_signal;
  action.sa_flags = SA_ONSTACK;
  sigemptyset(&action.sa_mask);
  if (sigaction(SIGUSR1, &action, NULL))
    return (void *)1;

  volatile char marker = 0;
  outer_address = (uintptr_t)&marker;
  int value = sigsetjmp(outer, 1);
  if (!value) {
    raise(SIGUSR1);
    return (void *)1;
  }
  if (value != 11 || !handler_ok)
    return (void *)1;
  stack_t disabled = {0};
  disabled.ss_flags = SS_DISABLE;
  if (sigaltstack(&disabled, NULL))
    return (void *)1;
  return NULL;
}

int main(void) {
  const size_t size = 1024 * 1024;
  char *memory = mmap(NULL, 2 * size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANON, -1, 0);
  if (memory == MAP_FAILED)
    return 1;
  struct stacks stacks = {memory + size, size};
  pthread_attr_t attr;
  pthread_t thread;
  if (pthread_attr_init(&attr) ||
      pthread_attr_setguardsize(&attr, 0) ||
      pthread_attr_setstack(&attr, memory, size) ||
      pthread_create(&thread, &attr, run, &stacks))
    return 1;
  pthread_attr_destroy(&attr);
  void *result;
  if (pthread_join(thread, &result) || result)
    return 1;
  if (munmap(memory, 2 * size))
    return 1;
  puts("Darwin live checkpoint across signal stacks: OK");
  return 0;
}

// CHECK: Darwin live checkpoint across signal stacks: OK
