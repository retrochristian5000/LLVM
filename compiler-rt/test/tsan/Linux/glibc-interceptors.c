// RUN: %clang_tsan -O1 -pthread %s -o %t
// RUN: %run %t 2>&1 | FileCheck %s --implicit-check-not='ThreadSanitizer'
// REQUIRES: linux
// UNSUPPORTED: android

// Regular pthread, fstat, and resolver calls must remain functional with the
// glibc-specific interceptors in a separate translation unit. Do not call
// glibc's private __pthread_mutex_* or __fxstat entry points directly.

#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
#if !defined(__GLIBC__)
  puts("glibc interceptors: SKIP");
#else
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  if (pthread_mutex_lock(&mutex) != 0)
    return 1;
  if (pthread_mutex_unlock(&mutex) != 0)
    return 2;
  if (pthread_mutex_destroy(&mutex) != 0)
    return 3;

  struct stat st;
  if (fstat(STDIN_FILENO, &st) != 0)
    return 4;

  struct addrinfo hints = {0};
  struct addrinfo *result = NULL;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  int rc = getaddrinfo("localhost", "80", &hints, &result);
  if (rc != 0)
    return 5;
  freeaddrinfo(result);
  puts("glibc interceptors: OK");
#endif
  return 0;
}

// CHECK: glibc interceptors: {{OK|SKIP}}
