#include "invalid_util.h"

#include <catch2/catch_test_macros.hpp>

#include <signal.h>

// In 32-bit mode 0x82 is an alias of the 0x80 group 1 encoding.
// In 64-bit mode it is an invalid opcode and must raise SIGILL,
// which this test verifies.

#define TEST(name, ...)                                                     \
  TEST_CASE("Signals: Invalid 0x82 " name) {                                \
    struct sigaction act {};                                                \
    act.sa_sigaction = CapturingHandler;                                    \
    act.sa_flags = SA_SIGINFO;                                              \
    sigaction(SIGILL, &act, nullptr);                                       \
                                                                            \
    from_handler.reset();                                                   \
    capturing_handler_skip = sizeof((const unsigned char[]) {__VA_ARGS__}); \
    __asm volatile(".byte " #__VA_ARGS__ ::: "memory");                     \
                                                                            \
    REQUIRE(from_handler.has_value());                                      \
    CHECK(from_handler->signal == SIGILL);                                  \
  }

// <op> byte [rax], 0x12
TEST("/0 add mem", 0x82, 0x00, 0x12);
TEST("/1 or mem", 0x82, 0x08, 0x12);
TEST("/2 adc mem", 0x82, 0x10, 0x12);
TEST("/3 sbb mem", 0x82, 0x18, 0x12);
TEST("/4 and mem", 0x82, 0x20, 0x12);
TEST("/5 sub mem", 0x82, 0x28, 0x12);
TEST("/6 xor mem", 0x82, 0x30, 0x12);
TEST("/7 cmp mem", 0x82, 0x38, 0x12);

// lock <op> byte [rax], 0x12
TEST("/0 lock add mem", 0xf0, 0x82, 0x00, 0x12);
TEST("/1 lock or mem", 0xf0, 0x82, 0x08, 0x12);
TEST("/2 lock adc mem", 0xf0, 0x82, 0x10, 0x12);
TEST("/3 lock sbb mem", 0xf0, 0x82, 0x18, 0x12);
TEST("/4 lock and mem", 0xf0, 0x82, 0x20, 0x12);
TEST("/5 lock sub mem", 0xf0, 0x82, 0x28, 0x12);
TEST("/6 lock xor mem", 0xf0, 0x82, 0x30, 0x12);
TEST("/7 lock cmp mem", 0xf0, 0x82, 0x38, 0x12);

// <op> al, 0x12
TEST("/0 add reg", 0x82, 0xc0, 0x12);
TEST("/1 or reg", 0x82, 0xc8, 0x12);
TEST("/2 adc reg", 0x82, 0xd0, 0x12);
TEST("/3 sbb reg", 0x82, 0xd8, 0x12);
TEST("/4 and reg", 0x82, 0xe0, 0x12);
TEST("/5 sub reg", 0x82, 0xe8, 0x12);
TEST("/6 xor reg", 0x82, 0xf0, 0x12);
TEST("/7 cmp reg", 0x82, 0xf8, 0x12);
