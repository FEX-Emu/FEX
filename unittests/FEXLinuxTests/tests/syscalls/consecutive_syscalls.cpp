#include <catch2/catch_test_macros.hpp>

#include <sys/syscall.h>
#include <unistd.h>

// Regression test for issue #5942. Not ending the block after a syscall
// can cause the register state to be incorrect afterwards, demonstrated here
// by running two syscalls in a row in the same block.
TEST_CASE("Two syscalls in one block") {
  long Result {};
#ifdef __x86_64__
  __asm volatile(R"(
    mov eax, %[GetPPid]
    syscall
    mov eax, %[GetPPid]
    syscall
  )"
                 : "=a"(Result)
                 : [GetPPid] "i"(SYS_getppid)
                 : "rcx", "r11", "memory");
#else
  __asm volatile(R"(
    mov eax, %[GetPPid]
    int 0x80
    mov eax, %[GetPPid]
    int 0x80
  )"
                 : "=a"(Result)
                 : [GetPPid] "i"(SYS_getppid)
                 : "memory");
#endif

  CHECK(Result == ::getppid());
}
