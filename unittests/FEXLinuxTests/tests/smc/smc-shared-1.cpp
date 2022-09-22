/*
    tests shared / mirrored mappings
*/

#include "smc-common.h"

#include <catch2/catch.hpp>

TEST_CASE("SMC: mmap_mremap") {
  auto code = (char *)mmap(0, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANON, 0, 0);

  auto code2 = (char *)mremap(code, 0, 4096, MREMAP_MAYMOVE);

  CHECK(test_shared(code, code2, "mmap_mremap") == 0);
}

TEST_CASE("SMC: mmap_mremap_mid") {
  auto code = (char *)mmap(0, 8192, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANON, 0, 0);

  auto code2 = (char *)mremap(code + 4096, 0, 4096, MREMAP_MAYMOVE);

  CHECK(test_shared(code + 4096, code2, "mmap_mremap_mid") == 0);
}

TEST_CASE("SMC: shmat") {
  auto shm = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0777);
  auto code3 = (char *)shmat(shm, nullptr, 0);
  auto code4 = (char *)shmat(shm, nullptr, SHM_EXEC);
  CHECK(test_shared(code3, code4, "shmat") == 0);
}

TEST_CASE("SMC: shmat_mremap") {
  auto shm2 = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0777);
  auto code5 = (char *)shmat(shm2, nullptr, SHM_EXEC);
  auto code6 = (char *)mremap(code5, 0, 4096, MREMAP_MAYMOVE);

  CHECK(test_shared(code5, code6, "shmat_mremap") == 0);
}

TEST_CASE("SMC: shmat_mremap_mid") {
  auto shm2 = shmget(IPC_PRIVATE, 8192, IPC_CREAT | 0777);
  auto code5 = (char *)shmat(shm2, nullptr, SHM_EXEC);
  auto code6 = (char *)mremap(code5 + 4096, 0, 4096, MREMAP_MAYMOVE);

  CHECK(test_shared(code5 + 4096, code6, "shmat_mremap_mid") == 0);
}

TEST_CASE("SMC: mmap_mmap") {
  char file[] = "smc-tests.XXXXXXXX";
  int fd = mkstemp(file);
  unlink(file);
  ftruncate(fd, 4096);

  auto code7 = (char *)mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  auto code8 = (char *)mmap(0, 4096, PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
  CHECK(test_shared(code7, code8, "mmap_mmap") == 0);
}

TEST_CASE("SMC: mmap_mmap_fd_fd2") {
  char file[] = "smc-tests.XXXXXXXX";
  int fd = mkstemp(file);
  int fd2 = open(file, O_RDONLY);
  unlink(file);
  ftruncate(fd, 4096);

  auto code = (char *)mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  auto code2 = (char *)mmap(0, 4096, PROT_READ | PROT_EXEC, MAP_SHARED, fd2, 0);
  CHECK(test_shared(code, code2, "mmap_mmap_fd_fd2") == 0);
}

TEST_CASE("SMC: shm_open_mmap_mmap") {
  char file[] = "smc-tests.XXXXXXXX";
  mktemp(file);
  int fd = shm_open(file, O_RDWR | O_CREAT, 0700);
  shm_unlink(file);
  ftruncate(fd, 4096);

  auto code7 = (char *)mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  auto code8 = (char *)mmap(0, 4096, PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
  CHECK(test_shared(code7, code8, "shm_open_mmap_mmap") == 0);
}

TEST_CASE("SMC: shm_open_mmap_mmap_fd_fd2") {
  char file[] = "smc-tests.XXXXXXXX";
  mktemp(file);
  int fd = shm_open(file, O_RDWR | O_CREAT, 0700);
  int fd2 = shm_open(file, O_RDONLY, 0700);
  shm_unlink(file);
  ftruncate(fd, 4096);

  auto code7 = (char *)mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  auto code8 = (char *)mmap(0, 4096, PROT_READ | PROT_EXEC, MAP_SHARED, fd2, 0);
  CHECK(test_shared(code7, code8, "shm_open_mmap_mmap_fd_fd2") == 0);
}
