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
