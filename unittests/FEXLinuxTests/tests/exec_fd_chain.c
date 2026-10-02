// SPDX-License-Identifier: MIT
// Three-stage x86 executable chain for the no-binfmt self-reexec path.
// The binary is deliberately outside the FEX RootFS. Each stage uses a
// caller-supplied argv[0] that differs from the executable path.

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char** environ;

static void fail(const char* message) {
  fprintf(stderr, "exec_fd_chain: %s\n", message);
  exit(1);
}

static int private_exec_env_leaked(void) {
  for (char** env = environ; env && *env; ++env) {
    if (strncmp(*env, "FEX_EXECVEFD=", 13) == 0 || strncmp(*env, "FEX_EXECVEFD_PATH=", 18) == 0) {
      return 1;
    }
  }
  return 0;
}

static void read_exe(char* path, size_t size) {
  ssize_t bytes = readlink("/proc/self/exe", path, size - 1);
  if (bytes < 0) {
    fail("readlink /proc/self/exe");
  }
  path[bytes] = '\0';
}

static void check_exe(const char* expected) {
  char path[4096];
  read_exe(path, sizeof(path));
  if (strcmp(path, expected) != 0) {
    fprintf(stderr, "exec_fd_chain: exe '%s' expected '%s'\n", path, expected);
    exit(1);
  }
}

int main(int argc, char** argv) {
  char self[4096];
  read_exe(self, sizeof(self));

  if (argc == 1) {
    char* child_argv[] = {"runtime-helper", "stage1", self, NULL};
    execve(self, child_argv, environ);
    perror("exec_fd_chain: execve stage1");
    return 1;
  }

  if (private_exec_env_leaked()) {
    fail("private exec fd environment leaked");
  }

  if (argc == 3 && strcmp(argv[1], "stage1") == 0) {
    if (strcmp(argv[0], "runtime-helper") != 0) {
      fprintf(stderr, "exec_fd_chain: stage1 argv0 '%s'\n", argv[0]);
      return 1;
    }
    check_exe(argv[2]);
    char* child_argv[] = {"worker", "stage2", argv[2], "sentinel", NULL};
    execve(self, child_argv, environ);
    perror("exec_fd_chain: execve stage2");
    return 1;
  }

  if (argc == 4 && strcmp(argv[1], "stage2") == 0) {
    if (strcmp(argv[0], "worker") != 0) {
      fprintf(stderr, "exec_fd_chain: stage2 argv0 '%s'\n", argv[0]);
      return 1;
    }
    if (strcmp(argv[3], "sentinel") != 0) {
      fail("missing sentinel");
    }
    check_exe(argv[2]);
    printf("exec_fd_chain=ok exe=%s argv0=%s\n", argv[2], argv[0]);
    return 0;
  }

  fprintf(stderr, "exec_fd_chain: unexpected argc %d\n", argc);
  return 1;
}
