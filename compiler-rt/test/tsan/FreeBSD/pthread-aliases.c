// RUN: %clang_tsan -O1 -pthread %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
// REQUIRES: freebsd

// FreeBSD's private libthr entry points must reuse the public pthread
// interceptors after their wrappers are moved into a separate translation unit.

#include <dlfcn.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

static int once_count;

static void do_once(void) { ++once_count; }

int main(void) {
  typedef int (*mutex_fn)(pthread_mutex_t *);
  typedef int (*once_fn)(pthread_once_t *, void (*)(void));
  typedef int (*sigmask_fn)(int, const sigset_t *, sigset_t *);

  mutex_fn lock = (mutex_fn)dlsym(RTLD_DEFAULT, "_pthread_mutex_lock");
  mutex_fn unlock = (mutex_fn)dlsym(RTLD_DEFAULT, "_pthread_mutex_unlock");
  once_fn once = (once_fn)dlsym(RTLD_DEFAULT, "_pthread_once");
  sigmask_fn sigmask = (sigmask_fn)dlsym(RTLD_DEFAULT, "_pthread_sigmask");
  if (!lock || !unlock || !once || !sigmask)
    return 1;

  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  if (lock(&mutex) != 0 || unlock(&mutex) != 0 ||
      pthread_mutex_destroy(&mutex) != 0)
    return 2;

  pthread_once_t control = PTHREAD_ONCE_INIT;
  if (once(&control, do_once) != 0 || once(&control, do_once) != 0 ||
      once_count != 1)
    return 3;

  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR2);
  if (sigmask(SIG_BLOCK, &mask, NULL) != 0 ||
      sigmask(SIG_UNBLOCK, &mask, NULL) != 0)
    return 4;

  puts("FreeBSD pthread aliases: OK");
  return 0;
}

// CHECK: FreeBSD pthread aliases: OK
