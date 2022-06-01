/*
  tests for smc changes memory mapped via mmap, mremap, shmat without mirroring
*/

auto args = "mmap, mremap, shmat, shmat_mremap, mmap_shmdt";

#include "smc-common.h"

int main(int argc, char *argv[]) {

  if (argc == 2) {
    if (strcmp(argv[1], "mmap") == 0) {
      auto code = (char *)mmap(0, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, 0, 0);
      return test(code, argv[1]);
    } else if (strcmp(argv[1], "mremap") == 0) {
      auto code = (char *)mmap(0, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANON, 0, 0);
      auto code2 = (char *)mremap(code, 0, 4096, MREMAP_MAYMOVE);
      return test(code2, argv[1]);
    } else if (strcmp(argv[1], "shmat") == 0) {
      auto shm = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0777);
      auto code = (char *)shmat(shm, nullptr, SHM_EXEC);
      return test(code, argv[1]);
    } else if (strcmp(argv[1], "shmat_mremap") == 0) {
      auto shm = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0777);
      auto code = (char *)shmat(shm, nullptr, SHM_EXEC);
      auto code2 = (char *)mremap(code, 0, 4096, MREMAP_MAYMOVE);
      return test(code2, argv[1]);
    } else if (strcmp(argv[1], "mmap_shmdt") == 0) {
      auto shmid = shmget(IPC_PRIVATE, 4096 * 3, IPC_CREAT | 0777);
      auto ptrshm = (char *)shmat(shmid, 0, 0);
      shmctl(shmid, IPC_RMID, NULL);
      auto ptrmmap = (char *)mmap(ptrshm + 4096, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_PRIVATE | MAP_ANON, 0, 0);
      shmdt(ptrshm);
      test(ptrmmap, argv[1]);
      return 0;
    }
  }

  printf("Invalid arguments\n");
  printf("please specify one of %s\n", args);

  return -1;
}
