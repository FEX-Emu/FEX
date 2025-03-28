#include <catch2/catch_test_macros.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>

struct compat_timeval {
  long tv_sec;
  long tv_usec;
};

uint64_t compat_futimesat(int dirfd, const char* pathname, const struct compat_timeval times[2]) {
  return ::syscall(SYS_futimesat, dirfd, pathname, times);
}

TEST_CASE("futimesat - invalid - minimum") {
  compat_timeval tvs[2] {};
  tvs[0].tv_sec = 0;
  tvs[0].tv_usec = -1;

  tvs[1].tv_sec = 0;
  tvs[1].tv_usec = -1;

  char file[] = "futimesat-tests.XXXXXXXX";
  int fd = mkstemp(file);
  REQUIRE(fd != -1);

  REQUIRE(compat_futimesat(fd, nullptr, tvs) == -1);
  CHECK(errno == EINVAL);
  REQUIRE(unlinkat(AT_FDCWD, file, 0) != -1);
  REQUIRE(close(fd) != -1);
}

TEST_CASE("futimesat - invalid - maximum") {
  compat_timeval tvs[2] {};
  tvs[0].tv_sec = 0;
  tvs[0].tv_usec = 1000000;

  tvs[1].tv_sec = 0;
  tvs[1].tv_usec = 1000000;

  char file[] = "futimesat-tests.XXXXXXXX";
  int fd = mkstemp(file);
  REQUIRE(fd != -1);

  REQUIRE(compat_futimesat(fd, nullptr, tvs) == -1);
  CHECK(errno == EINVAL);
  REQUIRE(unlinkat(AT_FDCWD, file, 0) != -1);
  REQUIRE(close(fd) != -1);
}

TEST_CASE("futimesat - valid - null") {
  char file[] = "futimesat-tests.XXXXXXXX";
  int fd = mkstemp(file);
  REQUIRE(fd != -1);

  timespec time {};
  REQUIRE(clock_gettime(CLOCK_REALTIME, &time) == 0);

  // Sets the time to "Now".
  REQUIRE(compat_futimesat(fd, nullptr, nullptr) == 0);
  REQUIRE(unlinkat(AT_FDCWD, file, 0) != -1);

  // Get the stat information of the file.
  struct stat sb {};
  REQUIRE(fstat(fd, &sb) == 0);
  CHECK(sb.st_atim.tv_sec >= time.tv_sec);
  CHECK(sb.st_mtim.tv_sec >= time.tv_sec);

  REQUIRE(close(fd) != -1);
}

TEST_CASE("futimesat - valid - future") {
  char file[] = "futimesat-tests.XXXXXXXX";
  int fd = mkstemp(file);
  REQUIRE(fd != -1);

  timespec time {};
  REQUIRE(clock_gettime(CLOCK_REALTIME, &time) == 0);

  compat_timeval tvs[2] {};
  tvs[0].tv_sec = time.tv_sec + 60;
  tvs[0].tv_usec = 0;

  tvs[1].tv_sec = time.tv_sec + 60;
  tvs[1].tv_usec = 0;

  // Sets the time to "Now".
  REQUIRE(compat_futimesat(fd, nullptr, tvs) == 0);
  REQUIRE(unlinkat(AT_FDCWD, file, 0) != -1);

  // Get the stat information of the file.
  struct stat sb {};
  REQUIRE(fstat(fd, &sb) == 0);
  CHECK(sb.st_atim.tv_sec == tvs[0].tv_sec);
  CHECK(sb.st_mtim.tv_sec == tvs[1].tv_sec);

  REQUIRE(close(fd) != -1);
}

TEST_CASE("futimesat - valid - past") {
  char file[] = "futimesat-tests.XXXXXXXX";
  int fd = mkstemp(file);
  REQUIRE(fd != -1);

  timespec time {};
  REQUIRE(clock_gettime(CLOCK_REALTIME, &time) == 0);

  compat_timeval tvs[2] {};
  tvs[0].tv_sec = time.tv_sec - 60;
  tvs[0].tv_usec = 0;

  tvs[1].tv_sec = time.tv_sec - 60;
  tvs[1].tv_usec = 0;

  // Sets the time to "Now".
  REQUIRE(compat_futimesat(fd, nullptr, tvs) == 0);
  REQUIRE(unlinkat(AT_FDCWD, file, 0) != -1);

  // Get the stat information of the file.
  struct stat sb {};
  REQUIRE(fstat(fd, &sb) == 0);
  CHECK(sb.st_atim.tv_sec == tvs[0].tv_sec);
  CHECK(sb.st_mtim.tv_sec == tvs[1].tv_sec);

  REQUIRE(close(fd) != -1);
}
