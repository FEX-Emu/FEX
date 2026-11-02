#include <catch2/catch_test_macros.hpp>

#include <array>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include <optional>

// FEX-Emu had a bug where partially decoded instruction faults weren't returning the correct data in the signal handler.
// Simple test that pushes a 7-byte NOP and a RET instruction across a RWX+RW page boundary to ensure correct fault addresses.
extern "C" {
__attribute__((naked, nocf_check)) void Ret() {
  __asm volatile(R"(
    .global RetInstruction
    RetInstruction:
    ret;
    )");
}

extern void* RetInstruction;
}

struct ReturnDataStruct {
  int Signal {};
  size_t FaultRIP {};
  size_t FaultAddr {};
};

std::optional<ReturnDataStruct> SignalReturnData {};

static void SignalHandler(int signal, siginfo_t* siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;

#ifdef REG_RIP
#define FEX_IP_REG REG_RIP
#else
#define FEX_IP_REG REG_EIP
#endif

  SignalReturnData = ReturnDataStruct {
    .Signal = signal,
    .FaultRIP = static_cast<size_t>(_context->uc_mcontext.gregs[FEX_IP_REG]),
    .FaultAddr = reinterpret_cast<size_t>(siginfo->si_addr),
  };

  _context->uc_mcontext.gregs[FEX_IP_REG] = reinterpret_cast<greg_t>(&RetInstruction);
#undef FEX_IP_REG
}

TEST_CASE("Partial Fault requires correct data") {
  struct sigaction Act {};
  Act.sa_sigaction = SignalHandler;
  Act.sa_flags = SA_SIGINFO;

  REQUIRE(sigaction(SIGILL, &Act, nullptr) == 0);
  REQUIRE(sigaction(SIGSEGV, &Act, nullptr) == 0);

  const auto PageSize = sysconf(_SC_PAGESIZE);
  REQUIRE(PageSize > 0);

  void* Ptr = mmap(nullptr, PageSize * 2, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  REQUIRE(Ptr != MAP_FAILED);

  REQUIRE(mprotect(static_cast<uint8_t*>(Ptr) + PageSize, PageSize, PROT_READ | PROT_WRITE) == 0);

  constexpr std::array<uint8_t, 8> NOPData = {
    0x0f, 0x18, 0x25, 0, 0, 0, 0, // nop dword [rip + Index]
    0xc3,                         // ret
  };

  struct ResultsStruct {
    bool ShouldFault {};
    std::optional<ReturnDataStruct> Result {};
  };

  std::array<ResultsStruct, NOPData.size() + 1> TestResults {{
    // Both instructions inside the executable page, won't fault.
    {false, std::nullopt},

    // Faults on the `ret` on the page boundary, which means FaultRIP == FaultAddr.
    {true,
     ReturnDataStruct {
       .Signal = SIGSEGV,
       .FaultRIP = reinterpret_cast<size_t>(Ptr) + PageSize,
       .FaultAddr = reinterpret_cast<size_t>(Ptr) + PageSize,
     }},
    // Starts faulting in the middle of nop instruction decode.
    // FaultAddr is always on the page boundary, but FaultRIP is where the instruction begins.
    {true,
     ReturnDataStruct {
       .Signal = SIGSEGV,
       .FaultRIP = reinterpret_cast<size_t>(Ptr) + PageSize - 6,
       .FaultAddr = reinterpret_cast<size_t>(Ptr) + PageSize,
     }},
    {true,
     ReturnDataStruct {
       .Signal = SIGSEGV,
       .FaultRIP = reinterpret_cast<size_t>(Ptr) + PageSize - 5,
       .FaultAddr = reinterpret_cast<size_t>(Ptr) + PageSize,
     }},
    {true,
     ReturnDataStruct {
       .Signal = SIGSEGV,
       .FaultRIP = reinterpret_cast<size_t>(Ptr) + PageSize - 4,
       .FaultAddr = reinterpret_cast<size_t>(Ptr) + PageSize,
     }},
    {true,
     ReturnDataStruct {
       .Signal = SIGSEGV,
       .FaultRIP = reinterpret_cast<size_t>(Ptr) + PageSize - 3,
       .FaultAddr = reinterpret_cast<size_t>(Ptr) + PageSize,
     }},
    {true,
     ReturnDataStruct {
       .Signal = SIGSEGV,
       .FaultRIP = reinterpret_cast<size_t>(Ptr) + PageSize - 2,
       .FaultAddr = reinterpret_cast<size_t>(Ptr) + PageSize,
     }},
    {true,
     ReturnDataStruct {
       .Signal = SIGSEGV,
       .FaultRIP = reinterpret_cast<size_t>(Ptr) + PageSize - 1,
       .FaultAddr = reinterpret_cast<size_t>(Ptr) + PageSize,
     }},
    // NOP Instruction is fully inside the RW page, so matches again.
    {true,
     ReturnDataStruct {
       .Signal = SIGSEGV,
       .FaultRIP = reinterpret_cast<size_t>(Ptr) + PageSize,
       .FaultAddr = reinterpret_cast<size_t>(Ptr) + PageSize,
     }},
  }};

  for (size_t i = 0; i < TestResults.size(); ++i) {
    auto& TestResult = TestResults[i];
    uint8_t* Code = static_cast<uint8_t*>(Ptr) + PageSize - NOPData.size() + i;
    memcpy(Code, NOPData.data(), NOPData.size());

    using FuncPtr = void (*)();
    auto Func = reinterpret_cast<FuncPtr>(Code);

    SignalReturnData.reset();
    Func();

    auto Result = SignalReturnData;

    REQUIRE(TestResult.ShouldFault == Result.has_value());
    if (TestResult.ShouldFault) {
      CHECK(TestResult.Result->Signal == Result->Signal);
      CHECK(TestResult.Result->FaultRIP == Result->FaultRIP);
      CHECK(TestResult.Result->FaultAddr == Result->FaultAddr);
    }
  }
}
