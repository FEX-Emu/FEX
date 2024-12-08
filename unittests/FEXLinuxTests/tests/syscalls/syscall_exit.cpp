#include <catch2/catch_test_macros.hpp>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/syscall.h>

TEST_CASE("fork - exit") {
  int child_pid = ::fork();
  if (child_pid == 0) {
    ::syscall(SYS_exit, 1);
    // unreachable
    std::terminate();
  } else {
    int status {};
    int exited_child = ::waitpid(child_pid, &status, 0);
    bool exited = WIFEXITED(status);
    REQUIRE(WIFEXITED(status) == 1);
    CHECK(WEXITSTATUS(status) == 1);
  }
}

TEST_CASE("fork - signal") {
  int child_pid = ::fork();
  if (child_pid == 0) {
    ::syscall(SYS_tgkill, ::getpid(), ::gettid(), SIGKILL);
    // unreachable
    std::terminate();
  } else {
    int status {};
    int exited_child = ::waitpid(child_pid, &status, 0);
    bool exited = WIFEXITED(status);
    REQUIRE(WIFSIGNALED(status) == 1);
    CHECK(WTERMSIG(status) == SIGKILL);
  }
}
