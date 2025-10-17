#include "simple_x86.h"
#include <catch2/catch_test_macros.hpp>

#include <signal.h>
#include <sys/mman.h>
#include <stdint.h>
#include <cstdlib>
#include <csetjmp>

bool Caught = false;
uint64_t CaughtAddr {};
static jmp_buf LongJump {};

static void SIGSEGV_Handler(int signal, siginfo_t* siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;
  // Needs to be an access error.
  REQUIRE(siginfo->si_code == SEGV_ACCERR);

  // Page fault
  REQUIRE(_context->uc_mcontext.gregs[REG_TRAPNO] == 14);

  CaughtAddr = reinterpret_cast<uint64_t>(siginfo->si_addr);

  Caught = true;
  longjmp(LongJump, 1);
}

TEST_CASE("Signals: Test No-Exec") {
  struct sigaction act {};
  act.sa_sigaction = SIGSEGV_Handler;
  act.sa_flags = SA_SIGINFO;

  sigaction(SIGSEGV, &act, nullptr);
  auto PageSize = sysconf(_SC_PAGESIZE);
  PageSize = PageSize > 0 ? PageSize : 0x1000;

  void* Ptr = mmap(nullptr, PageSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  REQUIRE(Ptr != MAP_FAILED);

  SimpleX86Emit emit(Ptr, PageSize);
  emit.mov(SimpleX86Emit::Reg::RAX, 1);
  emit.ret();

  using func_ptr = uint32_t (*)();
  func_ptr func = reinterpret_cast<func_ptr>(Ptr);

  // First time should execute fine.
  Caught = false;
  if (setjmp(LongJump) == 0) {
    int res = func();
    REQUIRE(res == 1);
  } else {
    REQUIRE(Caught == false);
  }

  // Protect as non-executable
  REQUIRE(mprotect(Ptr, PageSize, PROT_READ | PROT_WRITE) == 0);

  // This should now fail to execute due to No-Exec.
  Caught = false;
  if (setjmp(LongJump) == 0) {
    int res = func();
    // Shouldn't get reached.
    REQUIRE(res == 1);
    REQUIRE(false);
  } else {
    REQUIRE(Caught == true);
  }
}

TEST_CASE("Signals: Partial decode") {
  struct sigaction act {};
  act.sa_sigaction = SIGSEGV_Handler;
  act.sa_flags = SA_SIGINFO;

  sigaction(SIGSEGV, &act, nullptr);
  auto PageSize = sysconf(_SC_PAGESIZE);
  PageSize = PageSize > 0 ? PageSize : 0x1000;

  void* Ptr = mmap(nullptr, PageSize * 2, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  REQUIRE(Ptr != MAP_FAILED);

  SimpleX86Emit emit(static_cast<uint8_t*>(Ptr) + PageSize - 1, PageSize + 1);

  // MOV <uint32_t> hits the end of the page, writing only the opcode.
  emit.mov(SimpleX86Emit::Reg::RAX, 0x42424242);
  emit.ret();

  // Protect second page
  REQUIRE(mprotect(static_cast<uint8_t*>(Ptr) + PageSize, PageSize, PROT_NONE) == 0);

  using func_ptr = uint32_t (*)();
  func_ptr func = reinterpret_cast<func_ptr>(static_cast<uint8_t*>(Ptr) + PageSize - 1);
  Caught = false;
  if (setjmp(LongJump) == 0) {
    REQUIRE(func() != 0x42424242);
  } else {
    REQUIRE(Caught == true);
    CHECK(CaughtAddr == (reinterpret_cast<uint64_t>(Ptr) + PageSize));
  }

  Caught = false;
  CaughtAddr = 0;

  // Protect second page
  REQUIRE(mprotect(static_cast<uint8_t*>(Ptr) + PageSize, PageSize, PROT_READ | PROT_WRITE | PROT_EXEC) == 0);

  if (setjmp(LongJump) == 0) {
    CHECK(func() == 0x42424242);
  } else {
    REQUIRE(false);
  }

  CHECK(Caught == false);
  CHECK(CaughtAddr == 0);
}
