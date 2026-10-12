#include <catch2/catch_test_macros.hpp>

#include <cerrno>
#include <fcntl.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <unistd.h>

#define CHECK_EFAULT(Expr)  \
  do {                      \
    REQUIRE((Expr) == -1);  \
    CHECK(errno == EFAULT); \
  } while (0)

#define CHECK_ENOENT(Expr)  \
  do {                      \
    REQUIRE((Expr) == -1);  \
    CHECK(errno == ENOENT); \
  } while (0)

TEST_CASE("stat et al. returns EFAULT for null pointers") {
  int FD = ::open("/", O_RDONLY);
  CHECK_EFAULT(::syscall(SYS_oldstat, "/", nullptr));
  CHECK_EFAULT(::syscall(SYS_oldfstat, FD, nullptr));
  CHECK_EFAULT(::syscall(SYS_oldlstat, "/", nullptr));
  CHECK_EFAULT(::syscall(SYS_stat, "/", nullptr));
  CHECK_EFAULT(::syscall(SYS_fstat, FD, nullptr));
  CHECK_EFAULT(::syscall(SYS_lstat, "/", nullptr));
  CHECK_EFAULT(::syscall(SYS_stat64, "/", nullptr));
  CHECK_EFAULT(::syscall(SYS_lstat64, "/", nullptr));
  CHECK_EFAULT(::syscall(SYS_fstat64, FD, nullptr));
  CHECK_EFAULT(::syscall(SYS_statfs, "/", nullptr));
  CHECK_EFAULT(::syscall(SYS_fstatfs, FD, nullptr));
  CHECK_EFAULT(::syscall(SYS_fstatfs64, FD, sizeof(struct statfs64), nullptr));
  CHECK_EFAULT(::syscall(SYS_statfs64, "/", sizeof(struct statfs64), nullptr));
  CHECK_EFAULT(::syscall(SYS_fstatat64, AT_FDCWD, "/", nullptr, 0));
}

TEST_CASE("stat et al. returns ENOENT for missing paths with null buffers") {
  CHECK_ENOENT(::syscall(SYS_oldstat, "/does/not/exist", nullptr));
  CHECK_ENOENT(::syscall(SYS_oldlstat, "/does/not/exist", nullptr));
  CHECK_ENOENT(::syscall(SYS_stat, "/does/not/exist", nullptr));
  CHECK_ENOENT(::syscall(SYS_lstat, "/does/not/exist", nullptr));
  CHECK_ENOENT(::syscall(SYS_stat64, "/does/not/exist", nullptr));
  CHECK_ENOENT(::syscall(SYS_lstat64, "/does/not/exist", nullptr));
  CHECK_ENOENT(::syscall(SYS_statfs, "/does/not/exist", nullptr));
  CHECK_ENOENT(::syscall(SYS_statfs64, "/does/not/exist", sizeof(struct statfs64), nullptr));
  CHECK_ENOENT(::syscall(SYS_fstatat64, AT_FDCWD, "/does/not/exist", nullptr, 0));
}
