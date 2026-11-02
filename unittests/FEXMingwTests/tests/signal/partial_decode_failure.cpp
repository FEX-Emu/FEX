#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <unistd.h>
#include <cstdint>
#include <optional>
#include <windows.h>

// FEX-Emu had a bug where partially decoded instruction faults weren't returning the correct data in the signal handler.
// Simple test that pushes a 7-byte NOP and a RET instruction across a RWX+RW page boundary to ensure correct fault addresses.
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

extern void* PREFIX(RetInstruction);
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

  SignalReturnData = ReturnDataStruct {
    .Signal = ExceptionInfo->ExceptionRecord->ExceptionCode,
    .FaultRIP = RIP,
    .FaultAddr = ExceptionInfo->ExceptionRecord->ExceptionInformation[1],
  };

  RIP = reinterpret_cast<size_t>(&PREFIX(RetInstruction));
  return EXCEPTION_CONTINUE_EXECUTION;
}

TEST_CASE("Partial Fault requires correct data") {
  AddVectoredExceptionHandler(true, CapturingHandler);

  SYSTEM_INFO system_info {};
  GetSystemInfo(&system_info);

  const auto PageSize = system_info.dwPageSize;
  REQUIRE(PageSize > 0);

  void* Ptr = ::VirtualAlloc(nullptr, PageSize * 2, MEM_COMMIT | MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE);
  REQUIRE(Ptr != nullptr);

  DWORD OldProtect {};
  REQUIRE(::VirtualProtect(static_cast<uint8_t*>(Ptr) + PageSize, PageSize, PAGE_READWRITE, &OldProtect) != 0);

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
       .Signal = STATUS_ACCESS_VIOLATION,
       .FaultRIP = reinterpret_cast<size_t>(Ptr) + PageSize,
       .FaultAddr = reinterpret_cast<size_t>(Ptr) + PageSize,
     }},
    // Starts faulting in the middle of nop instruction decode.
    // FaultAddr is always on the page boundary, but FaultRIP is where the instruction begins.
    {true,
     ReturnDataStruct {
       .Signal = STATUS_ACCESS_VIOLATION,
       .FaultRIP = reinterpret_cast<size_t>(Ptr) + PageSize - 6,
       .FaultAddr = reinterpret_cast<size_t>(Ptr) + PageSize,
     }},
    {true,
     ReturnDataStruct {
       .Signal = STATUS_ACCESS_VIOLATION,
       .FaultRIP = reinterpret_cast<size_t>(Ptr) + PageSize - 5,
       .FaultAddr = reinterpret_cast<size_t>(Ptr) + PageSize,
     }},
    {true,
     ReturnDataStruct {
       .Signal = STATUS_ACCESS_VIOLATION,
       .FaultRIP = reinterpret_cast<size_t>(Ptr) + PageSize - 4,
       .FaultAddr = reinterpret_cast<size_t>(Ptr) + PageSize,
     }},
    {true,
     ReturnDataStruct {
       .Signal = STATUS_ACCESS_VIOLATION,
       .FaultRIP = reinterpret_cast<size_t>(Ptr) + PageSize - 3,
       .FaultAddr = reinterpret_cast<size_t>(Ptr) + PageSize,
     }},
    {true,
     ReturnDataStruct {
       .Signal = STATUS_ACCESS_VIOLATION,
       .FaultRIP = reinterpret_cast<size_t>(Ptr) + PageSize - 2,
       .FaultAddr = reinterpret_cast<size_t>(Ptr) + PageSize,
     }},
    {true,
     ReturnDataStruct {
       .Signal = STATUS_ACCESS_VIOLATION,
       .FaultRIP = reinterpret_cast<size_t>(Ptr) + PageSize - 1,
       .FaultAddr = reinterpret_cast<size_t>(Ptr) + PageSize,
     }},
    // NOP Instruction is fully inside the RW page, so matches again.
    {true,
     ReturnDataStruct {
       .Signal = STATUS_ACCESS_VIOLATION,
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
