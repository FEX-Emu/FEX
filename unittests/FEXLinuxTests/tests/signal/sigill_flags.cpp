#include <catch2/catch_test_macros.hpp>

#include <array>
#include <signal.h>
#include <sys/mman.h>
#include <stdint.h>
#include <cstdlib>
#include <optional>

__attribute__((naked, nocf_check)) static void SafeRet() {
  __asm volatile(R"(
  ret;
  )");
}

struct capture_data {
  uintptr_t RIP;
  uintptr_t siginfo_RIP;
  uint64_t Register_Err;
  uint64_t TrapNo;
};

std::optional<capture_data> data;
static void sigsegv_check(int signal, siginfo_t* siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;
  auto mcontext = &_context->uc_mcontext;

#ifdef REG_RIP
#define FEX_IP_REG REG_RIP
#else
#define FEX_IP_REG REG_EIP
#endif
  data.emplace(capture_data {
    .RIP = static_cast<uintptr_t>(mcontext->gregs[FEX_IP_REG]),
    .siginfo_RIP = reinterpret_cast<uintptr_t>(siginfo->si_addr),
    .Register_Err = static_cast<uint64_t>(mcontext->gregs[REG_ERR]),
    .TrapNo = static_cast<uint64_t>(mcontext->gregs[REG_TRAPNO]),
  });

  // Change RIP to a safe return so we can continue testing.
  mcontext->gregs[FEX_IP_REG] = reinterpret_cast<greg_t>(&SafeRet);
}

TEST_CASE("Signals: SIGILL flags") {
  struct sigaction act {};
  act.sa_sigaction = sigsegv_check;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);

  struct TestArray {
    int Prot;
    uint64_t RegisterErr;
  };

  constexpr static std::array<TestArray, 2> ProtArray {{
    {PROT_READ | PROT_WRITE, 21}, {PROT_READ, 21},

    // FEX doesn't currently support reporting this correctly.
    // { PROT_NONE, 20 },
  }};

  for (auto& Prot : ProtArray) {
    void* ptr = mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    REQUIRE(ptr != MAP_FAILED);

    // Fill JIT space with `ret` instruction.
    memset(ptr, 0xc3, 4096);

    // Protect with various NOEXEC protections.
    REQUIRE(mprotect(ptr, 4096, Prot.Prot) == 0);

    using func_type = void (*)();
    auto func = reinterpret_cast<func_type>(ptr);

    data.reset();

    // Jump to prepared JIT function with NOEXEC permissions.
    // Will immediately fault.
    func();

    REQUIRE(data.has_value());
    CHECK(data->RIP == reinterpret_cast<uintptr_t>(ptr));
    CHECK(data->siginfo_RIP == reinterpret_cast<uintptr_t>(ptr));
    CHECK(data->Register_Err == Prot.RegisterErr);
    CHECK(data->TrapNo == 14);
    REQUIRE(munmap(ptr, 4096) == 0);
  }
}
