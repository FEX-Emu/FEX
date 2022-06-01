
/*
    tests shared / mirrored mappings
*/

// libs: rt pthread

auto args = "mmap_fork, shmat_fork, fork_shmat_same_shmid";

#include "smc-common.h"

int main(int argc, char *argv[]) {

  if (argc == 2) {

    if (strcmp(argv[1], "mmap_fork") == 0) {
      auto code = (char *)mmap(0, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANON, 0, 0);

      return test_forked(code, code, argv[1]);
    } else if (strcmp(argv[1], "shmat_fork") == 0) {
      auto shm = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0777);
      auto code = (char *)shmat(shm, nullptr, SHM_EXEC);
      return test_forked(code, code, argv[1]);
    } else if (strcmp(argv[1], "fork_shmat_same_shmid") == 0) {
      auto shm = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0777);
      auto code3 = (char *)shmat(shm, nullptr, 0);
      if (fork() == 0) {
        auto code4 = (char *)shmat(shm, nullptr, SHM_EXEC);
        return test_shared(code3, code4, argv[1]);
      } else {
        int status;
        wait(&status);
        return WEXITSTATUS(status);
      }
    }
  }

  printf("Invalid arguments\n");
  printf("please specify one of %s\n", args);

  return -1;
}