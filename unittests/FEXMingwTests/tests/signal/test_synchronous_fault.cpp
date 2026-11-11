#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <unistd.h>
#include <cstdint>
#include <optional>
#include <windows.h>

// Checks various paths that FEX generates for synchronous exceptions to ensure the data is correct.
#ifdef __x86_64__
#define PREFIX(x) _##x
#else
#define PREFIX(x) x
#endif

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
  DWORD Signal {};
  size_t FaultRIP {};
  size_t FaultAddr {};
};

std::optional<ReturnDataStruct> SignalReturnData {};

__attribute__((stdcall)) static long CapturingHandler(struct _EXCEPTION_POINTERS* ExceptionInfo) {
#ifdef __x86_64__
  auto& RIP = ExceptionInfo->ContextRecord->Rip;
#else
  auto& RIP = ExceptionInfo->ContextRecord->Eip;
#endif

  ReturnDataStruct ReturnData {
    .Signal = ExceptionInfo->ExceptionRecord->ExceptionCode,
    .FaultRIP = RIP,
    .FaultAddr = {},
  };
  if (ExceptionInfo->ExceptionRecord->NumberParameters == 2) {
    ReturnData.FaultAddr = ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
  }

  SignalReturnData = ReturnData;

  RIP = reinterpret_cast<size_t>(&PREFIX(RetInstruction));
  return EXCEPTION_CONTINUE_EXECUTION;
}

TEST_CASE("Synchronous faults") {
  AddVectoredExceptionHandler(true, CapturingHandler);

  using PtrType = void (*)();
  struct ResultsStruct {
    PtrType Code {};
    ReturnDataStruct Result;
  };

  ResultsStruct TestResults[] = {
    {Do_UD2,
     ReturnDataStruct {
       .Signal = STATUS_ILLEGAL_INSTRUCTION,
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_UD2)),
       .FaultAddr = 0, // No fault addr here.
     }},
    {Do_INT1,
     ReturnDataStruct {
       .Signal = STATUS_SINGLE_STEP,
       // Fault RIP is RIP+Size
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_INT1)) + 1,
       .FaultAddr = 0, // No fault addr here.
     }},
    {Do_Long_INT1,
     ReturnDataStruct {
       .Signal = STATUS_SINGLE_STEP,
       // Fault RIP is RIP+Size
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_Long_INT1)) + 7,
       .FaultAddr = 0, // No fault addr here.
     }},
    {Do_INT3,
     ReturnDataStruct {
       .Signal = STATUS_BREAKPOINT,
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_INT3)),
       .FaultAddr = 0, // No fault addr here.
     }},
    {Do_Long_INT3,
     ReturnDataStruct {
       .Signal = STATUS_BREAKPOINT,
       // Fault RIP is at the fault instruction after the prefixes?
       // This might be a WINE bug.
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_Long_INT3)) + 6,
       .FaultAddr = 0, // No fault addr here.
     }},
    {Do_HLT,
     ReturnDataStruct {
       .Signal = STATUS_PRIVILEGED_INSTRUCTION,
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_HLT)),
       .FaultAddr = 0, // No fault addr here.
     }},
    {Do_Invalid_D6,
     ReturnDataStruct {
       .Signal = STATUS_ILLEGAL_INSTRUCTION,
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_Invalid_D6)),
       .FaultAddr = 0, // No fault addr here.
     }},
    {Do_MOVCS,
     ReturnDataStruct {
       .Signal = STATUS_ILLEGAL_INSTRUCTION,
       .FaultRIP = reinterpret_cast<size_t>(&PREFIX(Inst_MOVCS)),
       .FaultAddr = 0, // No fault addr here.
     }},
  };

  for (auto& TestResult : TestResults) {
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
