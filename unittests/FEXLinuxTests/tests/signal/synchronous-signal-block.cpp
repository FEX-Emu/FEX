auto args = "ssegv, asegv, sill, aill, sbus, abus, sfpe, afpe";

#include <cstring>
#include <errno.h>
#include <malloc.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define handle_error(msg)                                                                                                                  \
  do {                                                                                                                                     \
    perror(msg);                                                                                                                           \
    exit(EXIT_FAILURE);                                                                                                                    \
  } while (0)

char *buffer;
int flag = 0;

static void handler(int sig, siginfo_t *si, void *unused) {
  printf("Got %d at address: 0x%lx\n", sig, (long)si->si_addr);
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    printf("please specify one of %s\n", args);
  }

  struct sigaction sa;

  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = handler;
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGBUS, &sa, NULL);
  sigaction(SIGILL, &sa, NULL);
  sigaction(SIGFPE, &sa, NULL);

  auto map1 = mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
  auto map2 = (char *)mremap(map1, 4096, 8192, MREMAP_MAYMOVE);

  sigset_t set;
  sigfillset(&set);

  sigprocmask(SIG_SETMASK, &set, nullptr);

  if (strcmp(argv[1], "ssegv") == 0) {
    *(int *)(0x32) = 0x64;
  } else if (strcmp(argv[1], "sill") == 0) {
    asm volatile("ud2\n");
  } else if (strcmp(argv[1], "sbus") == 0) {
    map2[4096] = 2;
  } else if (strcmp(argv[1], "sfpe") == 0) {
    volatile int a = 10;
    volatile int b = 0;
    volatile int c = a / b;
    printf("result: %d\n", c);
  } else if (strcmp(argv[1], "asegv") == 0) {
    raise(SIGSEGV);
  } else if (strcmp(argv[1], "aill") == 0) {
    raise(SIGILL);
  } else if (strcmp(argv[1], "abus") == 0) {
    raise(SIGBUS);
  } else if (strcmp(argv[1], "afpe") == 0) {
    raise(SIGFPE);
  } else {
    printf("Invalid argument %s\n", argv[1]);
    printf("please specify one of %s\n", args);
  }

  exit(0);
}
