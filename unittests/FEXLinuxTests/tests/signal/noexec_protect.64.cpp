#include <catch2/catch_test_macros.hpp>

#include <signal.h>
#include <sys/mman.h>
#include <stdint.h>
#include <cstdlib>
#include <csetjmp>

bool Caught = false;
static jmp_buf LongJump {};

static void SIGSEGV_Handler(int signal, siginfo_t* siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;
  // Needs to be an access error.
  REQUIRE(siginfo->si_code == SEGV_ACCERR);

  // Page fault
  REQUIRE(_context->uc_mcontext.gregs[REG_TRAPNO] == 14);
  Caught = true;
  longjmp(LongJump, 1);
}

class SimpleX86Emit final {
public:
  enum Reg {
    RAX = 0,
    RCX = 1,
    RDX = 2,
    RBX = 3,
    RSP = 4,
    RBP = 5,
    RSI = 6,
    RDI = 7,
    // r8 and higher not implemented.
  };
  SimpleX86Emit(void* Ptr)
    : Ptr {static_cast<uint8_t*>(Ptr)} {}

  void ret() {
    db<uint8_t>(0xc3);
  }

  void mov(Reg reg, uint32_t val) {
    db<uint8_t>(0xB8 + reg);
    db(val);
  }
private:
  uint8_t* Ptr;

  template<typename T>
  void db(T v) {
    static_assert(sizeof(uint32_t) == 4);
    for (size_t i = 0; i < sizeof(T); ++i) {
      *Ptr = v >> (i * 8);
      ++Ptr;
    }
  }
};

TEST_CASE("Signals: Test No-Exec") {
  struct sigaction act {};
  act.sa_sigaction = SIGSEGV_Handler;
  act.sa_flags = SA_SIGINFO;

  sigaction(SIGSEGV, &act, nullptr);
  auto PageSize = sysconf(_SC_PAGESIZE);
  PageSize = PageSize > 0 ? PageSize : 0x1000;

  void* Ptr = mmap(nullptr, PageSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  REQUIRE(Ptr != MAP_FAILED);

  SimpleX86Emit emit(Ptr);
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
