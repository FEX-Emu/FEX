#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

#include <unistd.h>
#include <optional>
#include <sys/signal.h>

// Checks various paths that FEX generates for synchronous exceptions to ensure the data is correct.
#define PREFIX(x) _##x

extern "C" {
__attribute__((naked)) void Ret() {
  __asm volatile(R"(
    .global _RetInstruction
    _RetInstruction:
    ret;
    )");
}

__attribute__((naked)) void Do_UD2() {
  __asm volatile(R"(
    .global _Inst_UD2
    _Inst_UD2:
    ud2;
    ret;
    )");
}

__attribute__((naked)) void Do_INT1() {
  __asm volatile(R"(
    .global _Inst_INT1
    _Inst_INT1:
    .byte 0xF1; # int1;
    ret;
    )");
}
__attribute__((naked)) void Do_Long_INT1() {
  __asm volatile(R"(
    .global _Inst_Long_INT1
    _Inst_Long_INT1:
    .byte 0x66, 0x66, 0x66, 0x66, 0x66, 0x66;
    .byte 0xF1; # int1;
    ret;
    )");
}

__attribute__((naked)) void Do_INT3() {
  __asm volatile(R"(
    .global _Inst_INT3
    _Inst_INT3:
    int3;
    ret;
    )");
}

__attribute__((naked)) void Do_Long_INT3() {
  __asm volatile(R"(
    .global _Inst_Long_INT3
    _Inst_Long_INT3:
    .byte 0x66, 0x66, 0x66, 0x66, 0x66, 0x66;
    int3;
    ret;
    )");
}

__attribute__((naked)) void Do_HLT() {
  __asm volatile(R"(
    .global _Inst_HLT
    _Inst_HLT:
    hlt;
    ret;
    )");
}

// Technically an invalid encoding in the 3DNow! space.
__attribute__((naked)) void Do_Invalid_D6() {
  __asm volatile(R"(
    .global _Inst_Invalid_D6
    _Inst_Invalid_D6:
    .byte 0x0f, 0x0f, 0xd6;
    ret;
    )");
}

__attribute__((naked)) void Do_MOVCS() {
  __asm volatile(R"(
    .global _Inst_MOVCS
    _Inst_MOVCS:
    mov cs, ax;
    ret;
    )");
}

extern void* PREFIX(RetInstruction);

// Test instructions.
extern void* PREFIX(Inst_UD2);
extern void* PREFIX(Inst_INT1);
extern void* PREFIX(Inst_Long_INT1);
extern void* PREFIX(Inst_INT3);
extern void* PREFIX(Inst_Long_INT3);
extern void* PREFIX(Inst_HLT);
extern void* PREFIX(Inst_Invalid_D6);
extern void* PREFIX(Inst_MOVCS);
}

struct ReturnDataStruct {
  int Signal {};
  size_t FaultRIP {};
  size_t FaultAddr {};
};

std::optional<ReturnDataStruct> SignalReturnData {};

static void CapturingHandler(int signal, siginfo_t* siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;

#ifdef __x86_64__
  auto& RIP = _context->uc_mcontext.gregs[REG_RIP];
#else
  auto& RIP = _context->uc_mcontext.gregs[REG_EIP];
#endif

  ReturnDataStruct ReturnData {
    .Signal = signal,
    .FaultRIP = static_cast<size_t>(RIP),
    .FaultAddr = {},
  };

  ReturnData.FaultAddr = reinterpret_cast<size_t>(siginfo->si_addr);

  SignalReturnData = ReturnData;

  RIP = reinterpret_cast<size_t>(&PREFIX(RetInstruction));
}

TEST_CASE("Synchronous faults") {
  struct sigaction act {};
  act.sa_sigaction = CapturingHandler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);
  sigaction(SIGILL, &act, nullptr);
  sigaction(SIGTRAP, &act, nullptr);

  using PtrType = void (*)();
  struct ResultsStruct {
    const char* TestName {};
    PtrType Code {};
    ReturnDataStruct Result;
  };

  ResultsStruct TestResults[] = {
    {"UD2", Do_UD2,
     ReturnDataStruct {
       .Signal = SIGILL,
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_UD2)),
       .FaultAddr = reinterpret_cast<size_t>(&PREFIX(Inst_UD2)),
     }},
    {"Int1", Do_INT1,
     ReturnDataStruct {
       .Signal = SIGTRAP,
       // Fault RIP is RIP+Size
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_INT1)) + 1,
       .FaultAddr = reinterpret_cast<size_t>(&PREFIX(Inst_INT1)) + 1,
     }},
    {"Long Int1", Do_Long_INT1,
     ReturnDataStruct {
       .Signal = SIGTRAP,
       // Fault RIP is RIP+Size
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_Long_INT1)) + 7,
       .FaultAddr = reinterpret_cast<size_t>(&PREFIX(Inst_Long_INT1)) + 7,
     }},
    {"Int3", Do_INT3,
     ReturnDataStruct {
       .Signal = SIGTRAP,
       // Int3 is tricky and is the RIP after the instruction.
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_INT3)) + 1,
       .FaultAddr = 0,
     }},
    {"Long Int3", Do_Long_INT3,
     ReturnDataStruct {
       .Signal = SIGTRAP,
       // Int3 is tricky and is the RIP after the instruction.
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_Long_INT3)) + 7,
       .FaultAddr = 0,
     }},
    {"Hlt", Do_HLT,
     ReturnDataStruct {
       .Signal = SIGSEGV,
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_HLT)),
       .FaultAddr = 0,
     }},
    {"Invalid D6", Do_Invalid_D6,
     ReturnDataStruct {
       .Signal = SIGILL,
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_Invalid_D6)),
       .FaultAddr = reinterpret_cast<size_t>(&PREFIX(Inst_Invalid_D6)),
     }},
    {"Mov CS", Do_MOVCS,
     ReturnDataStruct {
       .Signal = SIGILL,
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_MOVCS)),
       .FaultAddr = reinterpret_cast<size_t>(&PREFIX(Inst_MOVCS)),
     }},
  };

  for (auto& TestResult : TestResults) {
    INFO("Test: " << TestResult.TestName);
    SignalReturnData.reset();

    // Execute it. This will fault and return.
    TestResult.Code();

    REQUIRE(SignalReturnData.has_value());
    auto Result = SignalReturnData;

    CHECK(TestResult.Result.Signal == Result->Signal);
    CHECK(TestResult.Result.FaultRIP == Result->FaultRIP);
    CHECK(TestResult.Result.FaultAddr == Result->FaultAddr);
  }
}
