#include <catch2/catch_test_macros.hpp>

#include <cerrno>
#include <fcntl.h>
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

// This should be more than big enough
static char Buffer[2048];

TEST_CASE("stat et al. returns EFAULT for null pointers") {
  int FD = ::open("/", O_RDONLY);
  CHECK_EFAULT(::syscall(SYS_stat, nullptr, Buffer));
  CHECK_EFAULT(::syscall(SYS_stat, "/", nullptr));
  CHECK_EFAULT(::syscall(SYS_fstat, FD, nullptr));
  CHECK_EFAULT(::syscall(SYS_lstat, nullptr, Buffer));
  CHECK_EFAULT(::syscall(SYS_lstat, "/", nullptr));
  CHECK_EFAULT(::syscall(SYS_newfstatat, AT_FDCWD, nullptr, Buffer, 0));
  CHECK_EFAULT(::syscall(SYS_newfstatat, AT_FDCWD, "/", nullptr, 0));
  CHECK_EFAULT(::syscall(SYS_statfs, nullptr, Buffer));
  CHECK_EFAULT(::syscall(SYS_statfs, "/", nullptr));
}

TEST_CASE("stat et al. returns ENOENT for missing paths with null buffers") {
  CHECK_ENOENT(::syscall(SYS_stat, "/does/not/exist", nullptr));
  CHECK_ENOENT(::syscall(SYS_lstat, "/does/not/exist", nullptr));
  CHECK_ENOENT(::syscall(SYS_newfstatat, AT_FDCWD, "/does/not/exist", nullptr, 0));
  CHECK_ENOENT(::syscall(SYS_statfs, "/does/not/exist", nullptr));
}
