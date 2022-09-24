/*
  tests for smc changes memory mapped via mmap, mremap, shmat without mirroring
*/

#include "smc-common.h"

#include <catch2/catch.hpp>

TEST_CASE("SMC: mmap") {
  auto code = (char *)mmap(0, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, 0, 0);
  CHECK(test(code, "mmap") == 0);
}

TEST_CASE("SMC: mremap") {
  auto code = (char *)mmap(0, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANON, 0, 0);
  auto code2 = (char *)mremap(code, 0, 4096, MREMAP_MAYMOVE);
  CHECK(test(code2, "mremap") == 0);
}

TEST_CASE("SMC: shmat") {
  auto shm = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0777);
  auto code = (char *)shmat(shm, nullptr, SHM_EXEC);
  CHECK(test(code, "shmat") == 0);
}

TEST_CASE("SMC: shmat_mremap") {
  auto shm = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0777);
  auto code = (char *)shmat(shm, nullptr, SHM_EXEC);
  auto code2 = (char *)mremap(code, 0, 4096, MREMAP_MAYMOVE);
  CHECK(test(code2, "shmat_mremap") == 0);
}

TEST_CASE("SMC: mmap_shmdt") {
  auto shmid = shmget(IPC_PRIVATE, 4096 * 3, IPC_CREAT | 0777);
  auto ptrshm = (char *)shmat(shmid, 0, 0);
  shmctl(shmid, IPC_RMID, NULL);
  auto ptrmmap = (char *)mmap(ptrshm + 4096, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_PRIVATE | MAP_ANON, 0, 0);
  shmdt(ptrshm);
  CHECK(test(ptrmmap, "mmap_shmdt") == 0);
}
