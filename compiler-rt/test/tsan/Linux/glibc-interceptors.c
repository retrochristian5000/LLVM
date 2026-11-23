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

  int pipefd[2];
  if (pipe(pipefd) != 0)
    return 4;
  struct stat st;
  if (fstat(pipefd[0], &st) != 0)
    return 5;
  if (close(pipefd[0]) != 0 || close(pipefd[1]) != 0)
    return 6;

  // Avoid DNS/network dependencies: exercise glibc's resolver-facing API
  // with an entirely numeric address and service.
  struct addrinfo hints = {0};
  struct addrinfo *result = NULL;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
  int rc = getaddrinfo("127.0.0.1", "80", &hints, &result);
  if (rc != 0)
    return 7;
  freeaddrinfo(result);
  puts("glibc interceptors: OK");
#endif
  return 0;
}

// CHECK: glibc interceptors: {{OK|SKIP}}
