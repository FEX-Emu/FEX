
/*
    tests shared / mirrored mappings
*/

#include "smc-common.h"

#include <catch2/catch.hpp>

TEST_CASE("SMC: mmap_fork") {
  auto code = (char *)mmap(0, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANON, 0, 0);
  CHECK(test_forked(code, code, "mmap_fork") == 0);
}

TEST_CASE("SMC: shmat_fork") {
  auto shm = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0777);
  auto code = (char *)shmat(shm, nullptr, SHM_EXEC);
  CHECK(test_forked(code, code, "shmat_fork") == 0);
}

TEST_CASE("SMC: fork_shmat_same_shmid") {
  auto shm = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0777);
  auto code3 = (char *)shmat(shm, nullptr, 0);
  // NOTE: Forking in a test will fork the entire Catch2 test runtime.
  //       That's not great, but it doesn't seem to cause any issues other
  //       than printing test results twice
  if (fork() == 0) {
    auto code4 = (char *)shmat(shm, nullptr, SHM_EXEC);
    CHECK(test_shared(code3, code4, "fork_shmat_same_shmid") == 0);
  } else {
    int status;
    wait(&status);
    CHECK(WEXITSTATUS(status) == 0);
  }
}
