#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>

static std::vector<char> LoadFile(const char* Path) {
  int fd = open(Path, O_RDONLY);
  REQUIRE(fd != -1);

  struct stat st {};
  REQUIRE(fstat(fd, &st) != -1);

  std::vector<char> Result {};
  Result.resize(st.st_size);

  size_t DidRead {};
  do {
    auto Read = read(fd, Result.data() + DidRead, Result.size() - DidRead);

    if (Read == -1) {
      if (errno == EINTR || errno == EAGAIN) {
        continue;
      }
      REQUIRE(errno != 0);
    }

    DidRead += Read;
  } while (DidRead != st.st_size);

  return Result;
}

TEST_CASE("execveat - memfd - MFD_CLOEXEC") {
  auto MapsFile = LoadFile("/usr/bin/true");
  REQUIRE(MapsFile.size() != 0);

  int fd = memfd_create("Anonymous", MFD_CLOEXEC | MFD_ALLOW_SEALING);
  REQUIRE(fd != -1);

  size_t Written {};
  do {
    auto Wrote = write(fd, MapsFile.data() + Written, MapsFile.size() - Written);
    if (Wrote == -1) {
      if (errno == EINTR || errno == EAGAIN) {
        continue;
      }
      REQUIRE(errno != 0);
    }
    Written += Wrote;
  } while (Written != MapsFile.size());

  const char* argv[] = {"tmp", nullptr};
  auto Res = ::syscall(SYS_execveat, fd, "", argv, nullptr, AT_EMPTY_PATH);

  // Will only get here if execveat fails.
  close(fd);
  REQUIRE(Res == 0);
}
