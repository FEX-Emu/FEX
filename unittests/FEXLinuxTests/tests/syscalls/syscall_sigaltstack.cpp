#include <catch2/catch_test_macros.hpp>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/syscall.h>

TEST_CASE("sysaltstack - minimum") {
  char test[4096];
  constexpr size_t EXPECTED_MIN = 2048;

  stack_t stack {
    .ss_sp = test,
    .ss_flags = 0,
    .ss_size = 0,
  };
  for (size_t i = 1; i < sizeof(test); ++i) {
    stack.ss_size = i;
    CHECK(sigaltstack(&stack, nullptr) == (i < EXPECTED_MIN ? -1 : 0));
  }
}
