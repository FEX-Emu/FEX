// SPDX-License-Identifier: MIT

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cassert>
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <errhandlingapi.h>
#include <processthreadsapi.h>
#include <windef.h>
#include <winbase.h>
#include <winternl.h>
#include <winnt.h>
#include <ntstatus.h>
#include <thread>

#ifdef __x86_64__
#define ASM_CONV
#else
#define ASM_CONV fastcall
#endif
#define str(x) #x
#define xstr(x) str(x)

// Debug registers on x86 are fairly flexible but there are few of them.
// - DR0-3 are registers which contain the addresses to breakpoint on.
// - DR4/5 are reserved. Aliased to DR6/DR7. Attempting to access will fault.
// - DR6 is `Debug-Status Register` containing sticky bits about what conditions occured.
// - DR7 is `Debug-Control Register` which controls how the breakpoints are controlled.
// - D8-15 are defined for x86_64 but are currently reserved.

enum LengthClass : uint32_t {
  Byte = 0,
  Word = 1,
  QWord = 2,
  DWord = 3,
};
enum TypeClass : uint32_t {
  InstExec = 0,
  DataWrite = 1,
  IOReadWrite = 2,
  DataReadWrite = 3,
};

struct BreakpointAssignments {
  struct BPInfo {
    size_t Addr;
    LengthClass Length;
    TypeClass TypeOfFault;
    bool LocalBPEnable;
    bool GlobalBPEnable;
  };
  BPInfo BPs[4] {};
  size_t DebugStatusRegister {};
  // Used to detect when any `MOV DRn` occurs.
  // Sets DR6[BD] to 1 when occurs.
  // Since `MOV DRn` isn't available to CPL3, it isn't interesting although.
  bool GeneralDetectEnabled;
};

#ifdef __x86_64__
#define PREFIX(x) _##x
#else
#define PREFIX(x) x
#endif

extern "C" void* PREFIX(ReadBPState);
extern "C" void* PREFIX(ReturnReadBPState);

extern "C" void* PREFIX(WriteBPState);
extern "C" void* PREFIX(ReturnWriteBPState);

extern "C" void* PREFIX(BPFaultContinueInstruction);
extern "C" void* PREFIX(ReturnBPFaultContinueInstruction);

extern "C" void* PREFIX(BPFaultInstruction);
extern "C" void* PREFIX(ReturnBPFaultInstruction);

union DataStruct {
  struct {
    uint8_t _pad[7];
    uint8_t u8;
  };
  struct {
    uint16_t _pad2[3];
    uint16_t u16;
  };
  struct {
    uint32_t _pad3;
    uint32_t u32;
  };
  uint64_t u64;
};

static DataStruct FaultData {};
// Have number of exceptions after fault data to capture any BP reads that extend too far.
static size_t NumExceptions {};

void FetchBPData(BreakpointAssignments* bp, const PCONTEXT ctx) {
  bp->BPs[0].Addr = ctx->Dr0;
  bp->BPs[1].Addr = ctx->Dr1;
  bp->BPs[2].Addr = ctx->Dr2;
  bp->BPs[3].Addr = ctx->Dr3;
  bp->DebugStatusRegister = ctx->Dr6;

  const size_t Dr7 = ctx->Dr7;
  // Local and global for each BP is sequential.
  bp->BPs[0].LocalBPEnable = !!(Dr7 & (1 << 0));
  bp->BPs[0].GlobalBPEnable = !!(Dr7 & (1 << 1));
  bp->BPs[1].LocalBPEnable = !!(Dr7 & (1 << 2));
  bp->BPs[1].GlobalBPEnable = !!(Dr7 & (1 << 3));
  bp->BPs[2].LocalBPEnable = !!(Dr7 & (1 << 4));
  bp->BPs[2].GlobalBPEnable = !!(Dr7 & (1 << 5));
  bp->BPs[3].LocalBPEnable = !!(Dr7 & (1 << 6));
  bp->BPs[3].GlobalBPEnable = !!(Dr7 & (1 << 7));

  // Then general detect bit.
  bp->GeneralDetectEnabled = !!(Dr7 & (1 << 13));

  // Then Lengths and types.
  bp->BPs[0].TypeOfFault = static_cast<TypeClass>((Dr7 >> 16) & 0b11);
  bp->BPs[0].Length = static_cast<LengthClass>((Dr7 >> 18) & 0b11);
  bp->BPs[1].TypeOfFault = static_cast<TypeClass>((Dr7 >> 20) & 0b11);
  bp->BPs[1].Length = static_cast<LengthClass>((Dr7 >> 22) & 0b11);
  bp->BPs[2].TypeOfFault = static_cast<TypeClass>((Dr7 >> 24) & 0b11);
  bp->BPs[2].Length = static_cast<LengthClass>((Dr7 >> 26) & 0b11);
  bp->BPs[3].TypeOfFault = static_cast<TypeClass>((Dr7 >> 28) & 0b11);
  bp->BPs[3].Length = static_cast<LengthClass>((Dr7 >> 30) & 0b11);
}

void WriteBPData(const BreakpointAssignments* bp, PCONTEXT ctx) {
  ctx->Dr0 = bp->BPs[0].Addr;
  ctx->Dr1 = bp->BPs[1].Addr;
  ctx->Dr2 = bp->BPs[2].Addr;
  ctx->Dr3 = bp->BPs[3].Addr;
  ctx->Dr6 = bp->DebugStatusRegister;

  size_t Dr7 {};
  // Local and global for each BP is sequential.
  Dr7 |= bp->BPs[0].LocalBPEnable ? (1 << 0) : 0;
  Dr7 |= bp->BPs[0].GlobalBPEnable ? (1 << 1) : 0;
  Dr7 |= bp->BPs[1].LocalBPEnable ? (1 << 2) : 0;
  Dr7 |= bp->BPs[1].GlobalBPEnable ? (1 << 3) : 0;
  Dr7 |= bp->BPs[2].LocalBPEnable ? (1 << 4) : 0;
  Dr7 |= bp->BPs[2].GlobalBPEnable ? (1 << 5) : 0;
  Dr7 |= bp->BPs[3].LocalBPEnable ? (1 << 6) : 0;
  Dr7 |= bp->BPs[3].GlobalBPEnable ? (1 << 7) : 0;

  // Then general detect bit.
  Dr7 |= bp->GeneralDetectEnabled ? (1 << 13) : 0;

  // Then Lengths and types.
  Dr7 |= (bp->BPs[0].TypeOfFault << 16);
  Dr7 |= (bp->BPs[0].Length << 18);
  Dr7 |= (bp->BPs[1].TypeOfFault << 20);
  Dr7 |= (bp->BPs[1].Length << 22);
  Dr7 |= (bp->BPs[2].TypeOfFault << 24);
  Dr7 |= (bp->BPs[2].Length << 26);
  Dr7 |= (bp->BPs[3].TypeOfFault << 28);
  Dr7 |= (bp->BPs[3].Length << 30);

  ctx->Dr7 = Dr7;
}

__attribute__((stdcall)) static long CapturingHandler(EXCEPTION_POINTERS* ExceptionInfo) {
#ifdef __x86_64__
#define ARG1 Rcx
#define ARGRET Rax
  auto& RIP = ExceptionInfo->ContextRecord->Rip;
#else
#define ARG1 Ecx
#define ARGRET Eax
  auto& RIP = ExceptionInfo->ContextRecord->Eip;
#endif
  ++NumExceptions;

  fprintf(stderr, "Exception at 0x%zx\n", (size_t)RIP);
  if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_ILLEGAL_INSTRUCTION) {
    if (RIP == reinterpret_cast<size_t>(&PREFIX(ReadBPState))) {
      auto BPArg = reinterpret_cast<BreakpointAssignments*>(ExceptionInfo->ContextRecord->ARG1);
      if (ExceptionInfo->ContextRecord->ContextFlags & CONTEXT_DEBUG_REGISTERS) {
        FetchBPData(BPArg, ExceptionInfo->ContextRecord);
      } else {
        fprintf(stderr, "Debug registers were empty\n");
      }
      RIP = reinterpret_cast<size_t>(&PREFIX(ReturnReadBPState));
      return EXCEPTION_CONTINUE_EXECUTION;
    }

    if (RIP == reinterpret_cast<size_t>(&PREFIX(WriteBPState))) {
      auto BPArg = reinterpret_cast<BreakpointAssignments*>(ExceptionInfo->ContextRecord->ARG1);
      if (ExceptionInfo->ContextRecord->ContextFlags & CONTEXT_DEBUG_REGISTERS) {
        WriteBPData(BPArg, ExceptionInfo->ContextRecord);
      } else {
        fprintf(stderr, "Debug registers were empty\n");
      }
      RIP = reinterpret_cast<size_t>(&PREFIX(ReturnWriteBPState));
      return EXCEPTION_CONTINUE_EXECUTION;
    }
  } else if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP) {
    // Only skip when on `BPFaultContinue`.
    // Explicitly avoiding handling on `BPFault`
    fprintf(stderr, "Return reg is: 0x%zx\n", (size_t)ExceptionInfo->ContextRecord->ARGRET);
    constexpr uint32_t MeaningfulBits = (0b111 << 13) | (1 << 1) | (0xF);
    fprintf(stderr, "Dr6: 0x%zx\n", (size_t)ExceptionInfo->ContextRecord->Dr6 & MeaningfulBits);

    if (ExceptionInfo->ContextRecord->Dr0 == RIP) {
      fprintf(stderr, "Exception at Dr0: 0x%zx\n", (size_t)ExceptionInfo->ContextRecord->Dr7);
      ExceptionInfo->ContextRecord->Dr7 &= ~(0b11 << 0);
    }
    if (ExceptionInfo->ContextRecord->Dr1 == RIP) {
      fprintf(stderr, "Exception at Dr1: 0x%zx\n", (size_t)ExceptionInfo->ContextRecord->Dr7);
      ExceptionInfo->ContextRecord->Dr7 &= ~(0b11 << 2);
    }
    if (ExceptionInfo->ContextRecord->Dr2 == RIP) {
      fprintf(stderr, "Exception at Dr2: 0x%zx\n", (size_t)ExceptionInfo->ContextRecord->Dr7);
      ExceptionInfo->ContextRecord->Dr7 &= ~(0b11 << 4);
    }
    if (ExceptionInfo->ContextRecord->Dr3 == RIP) {
      fprintf(stderr, "Exception at Dr3: 0x%zx\n", (size_t)ExceptionInfo->ContextRecord->Dr7);
      ExceptionInfo->ContextRecord->Dr7 &= ~(0b11 << 6);
    }

    if (RIP == reinterpret_cast<size_t>(&PREFIX(BPFaultContinueInstruction))) {
      RIP = reinterpret_cast<size_t>(&PREFIX(ReturnBPFaultContinueInstruction));
    }
    return EXCEPTION_CONTINUE_EXECUTION;
  }

  fprintf(stderr, "Unhandled exception: 0x%lx\n", ExceptionInfo->ExceptionRecord->ExceptionCode);
  assert(false);
  return EXCEPTION_CONTINUE_SEARCH;
}

void InstallSignalHandler() {
  AddVectoredExceptionHandler(true, CapturingHandler);
}

__attribute__((naked, ASM_CONV)) void ReadBreakpoints(BreakpointAssignments* BP) {
  asm(R"(
  .global _ReadBPState
  _ReadBPState:
    ud2;

  .global _ReturnReadBPState;
  _ReturnReadBPState:
  ret;
  )" ::
        : "memory");
}
__attribute__((naked, ASM_CONV)) void WriteBreakpoints(const BreakpointAssignments* BP) {
  asm(R"(
  .global _WriteBPState
  _WriteBPState:
    ud2;

  .global _ReturnWriteBPState;
  _ReturnWriteBPState:
  ret;
  )" ::
        : "memory");
}
__attribute__((naked, ASM_CONV)) uint32_t BPFaultContinueFunction() {
  asm(R"(
  mov eax, 0xdeadbeef;

  mov eax, 1;

.global _BPFaultContinueInstruction
_BPFaultContinueInstruction:
  mov eax, 0;

.global _ReturnBPFaultContinueInstruction;
_ReturnBPFaultContinueInstruction:
ret;
)" ::
        : "memory");
}

__attribute__((naked, ASM_CONV)) uint32_t BPFaultFunction() {
  asm(R"(
  mov eax, 0xdeadbeef;

.global _BPFaultInstruction
_BPFaultInstruction:
  mov eax, 1;

.global ReturnBPFaultInstruction;
ReturnBPFaultInstruction:
ret;
)" ::
        : "memory");
}

static DWORD WINAPI empty(void* arg) {
  return 0;
}

// Check if a created thread with breakpoint data matches.
void test0() {
  CONTEXT ctx {};
  ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
  ctx.Dr0 = ~0U;
  ctx.Dr1 = ~0U;
  ctx.Dr2 = ~0U;
  ctx.Dr3 = ~0U;
  ctx.Dr6 = ~0U;
  ctx.Dr7 = 0x400;

  NTSTATUS status {};

  auto t = CreateThread(nullptr, 0, empty, nullptr, CREATE_SUSPENDED, nullptr);
  REQUIRE(t != INVALID_HANDLE_VALUE);

  ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
  bool success = SetThreadContext(t, &ctx);
  REQUIRE(success);

  success = GetThreadContext(t, &ctx);
  REQUIRE(success);

  CHECK(ctx.Dr0 == ~0U);
  CHECK(ctx.Dr1 == ~0U);
  CHECK(ctx.Dr2 == ~0U);
  CHECK(ctx.Dr3 == ~0U);
  CHECK(ctx.Dr6 == ~0U);
  CHECK(ctx.Dr7 == 0x400);
}

// Ensures by default the debug registers are empty.
void test1() {
  NumExceptions = 0;
  BreakpointAssignments bp {};
  memset(&bp, 0x42, sizeof(bp));

  // Ensure breakpoints are empty.
  ReadBreakpoints(&bp);

  REQUIRE(bp.GeneralDetectEnabled == false);
  REQUIRE(bp.DebugStatusRegister == 0);
  for (size_t i = 0; i < 4; ++i) {
    REQUIRE(bp.BPs[i].Addr == 0);
    REQUIRE(bp.BPs[i].Length == LengthClass::Byte);
    REQUIRE(bp.BPs[i].TypeOfFault == TypeClass::InstExec);
    REQUIRE(bp.BPs[i].LocalBPEnable == false);
    REQUIRE(bp.BPs[i].GlobalBPEnable == false);
  }
  REQUIRE(NumExceptions == 1);
}

// Ensures the BP addresses are sticky.
void test2() {
  NumExceptions = 0;
  BreakpointAssignments bp {};
  bp.BPs[0].Addr = 0x41424344;
  bp.BPs[1].Addr = 0x51525354;
  bp.BPs[2].Addr = 0x61626364;
  bp.BPs[3].Addr = 0x71727374;

  // Write a breakpoint.
  WriteBreakpoints(&bp);

  memset(&bp, 0x42, sizeof(bp));

  // Read it back.
  ReadBreakpoints(&bp);

  // Ensure the addresses are sticky.
  CHECK(bp.BPs[0].Addr == 0x41424344);
  CHECK(bp.BPs[1].Addr == 0x51525354);
  CHECK(bp.BPs[2].Addr == 0x61626364);
  CHECK(bp.BPs[3].Addr == 0x71727374);

  REQUIRE(bp.GeneralDetectEnabled == false);
  REQUIRE(bp.DebugStatusRegister == 0);

  for (size_t i = 0; i < 4; ++i) {
    REQUIRE(bp.BPs[i].Length == LengthClass::Byte);
    REQUIRE(bp.BPs[i].TypeOfFault == TypeClass::InstExec);
    REQUIRE(bp.BPs[i].LocalBPEnable == false);
    REQUIRE(bp.BPs[i].GlobalBPEnable == false);
  }

  REQUIRE(NumExceptions == 2);
}

bool GlobalBPSupported = true;
bool GeneralDetectedSupported = true;

// Ensures the BP options are sticky.
void test3() {
  NumExceptions = 0;
  BreakpointAssignments bp {};
  bp.GeneralDetectEnabled = true;
  for (size_t i = 0; i < 4; ++i) {
    bp.BPs[i].LocalBPEnable = true;
    bp.BPs[i].GlobalBPEnable = true;
  }

  // Write a breakpoint.
  WriteBreakpoints(&bp);

  memset(&bp, 0x42, sizeof(bp));

  // Read it back.
  ReadBreakpoints(&bp);

  // Windows doesn't support setting General detect.
  GeneralDetectedSupported = bp.GeneralDetectEnabled;
  REQUIRE(bp.DebugStatusRegister == 0);
  for (size_t i = 0; i < 4; ++i) {
    REQUIRE(bp.BPs[i].Addr == 0);
    REQUIRE(bp.BPs[i].Length == LengthClass::Byte);
    REQUIRE(bp.BPs[i].TypeOfFault == TypeClass::InstExec);
    CHECK(bp.BPs[i].LocalBPEnable == true);

    // Windows doesn't support global BP.
    if (bp.BPs[i].GlobalBPEnable == false) {
      GlobalBPSupported = false;
    }
  }

  REQUIRE(NumExceptions == 2);
}

// Spin up a new thread and wait on it.
// Ensures that breakpoint information isn't inherited.
void test4() {
  // Set the addresses from test2().
  test2();

  std::thread t([]() { test1(); });
  t.join();
}

// Ensures that when a instruction bp is hit, the signal handler can jump to a new instruction.
// Skipping one.
void test5(int BP, bool Global) {
  if (Global && !GlobalBPSupported) {
    return;
  }
  NumExceptions = 0;
  BreakpointAssignments bp {};
  bp.BPs[BP].Addr = reinterpret_cast<size_t>(&PREFIX(BPFaultContinueInstruction));
  bp.BPs[BP].GlobalBPEnable = Global;
  bp.BPs[BP].LocalBPEnable = !Global;

  static int count {};
  fprintf(stderr, "# Test5: %d\n", ++count);

  fprintf(stderr, "## WriteBP: Fault RIP: 0x%zx\n", bp.BPs[BP].Addr);

  // Write a breakpoint.
  WriteBreakpoints(&bp);

  fprintf(stderr, "### Faulting\n");

  uint32_t Ret = BPFaultContinueFunction();
  REQUIRE(Ret == 1);

  fprintf(stderr, "#### Reading BP\n");

  // Read it back.
  ReadBreakpoints(&bp);

  REQUIRE(bp.GeneralDetectEnabled == false);
  constexpr uint32_t MeaningfulBits = (0b111 << 13) | (1 << 1) | (0xF);

  // Only bp0 fired.
  REQUIRE((bp.DebugStatusRegister & MeaningfulBits) == (1 << BP));

  REQUIRE(bp.BPs[BP].Addr == reinterpret_cast<size_t>(&PREFIX(BPFaultContinueInstruction)));
  REQUIRE(bp.BPs[BP].Length == LengthClass::Byte);
  REQUIRE(bp.BPs[BP].TypeOfFault == TypeClass::InstExec);

  // These will have fired and changed state.
  REQUIRE(bp.BPs[BP].GlobalBPEnable == false);
  REQUIRE(bp.BPs[BP].LocalBPEnable == false);

  for (size_t i = 0; i < 4; ++i) {
    if (i == BP) {
      continue;
    }

    REQUIRE(bp.BPs[i].Addr == 0);
    REQUIRE(bp.BPs[i].Length == LengthClass::Byte);
    REQUIRE(bp.BPs[i].TypeOfFault == TypeClass::InstExec);
    REQUIRE(bp.BPs[i].LocalBPEnable == false);
    REQUIRE(bp.BPs[i].GlobalBPEnable == false);
  }

  REQUIRE(NumExceptions == 3);
}

// Ensures that when a instruction BP is hit, it continues execution afterwards.
void test6(int BP, bool Global) {
  if (Global && !GlobalBPSupported) {
    return;
  }

  NumExceptions = 0;
  BreakpointAssignments bp {};
  bp.BPs[BP].Addr = reinterpret_cast<size_t>(&PREFIX(BPFaultInstruction));
  bp.BPs[BP].GlobalBPEnable = Global;
  bp.BPs[BP].LocalBPEnable = !Global;

  // Write a breakpoint.
  WriteBreakpoints(&bp);

  uint32_t Ret = BPFaultFunction();
  REQUIRE(Ret == 1);

  memset(&bp, 0x42, sizeof(bp));

  // Read it back.
  ReadBreakpoints(&bp);

  REQUIRE(bp.GeneralDetectEnabled == false);
  constexpr uint32_t MeaningfulBits = (0b111 << 13) | (1 << 1) | (0xF);

  // Only bp0 fired.
  CHECK((bp.DebugStatusRegister & MeaningfulBits) == (1 << BP));

  CHECK(bp.BPs[BP].Addr == reinterpret_cast<size_t>(&PREFIX(BPFaultInstruction)));
  CHECK(bp.BPs[BP].Length == LengthClass::Byte);
  CHECK(bp.BPs[BP].TypeOfFault == TypeClass::InstExec);

  // These will have fired and changed state.
  REQUIRE(bp.BPs[BP].GlobalBPEnable == false);
  REQUIRE(bp.BPs[BP].LocalBPEnable == false);

  for (size_t i = 0; i < 4; ++i) {
    if (i == BP) {
      continue;
    }

    CHECK(bp.BPs[i].Addr == 0);
    CHECK(bp.BPs[i].Length == LengthClass::Byte);
    CHECK(bp.BPs[i].TypeOfFault == TypeClass::InstExec);
    CHECK(bp.BPs[i].LocalBPEnable == false);
    CHECK(bp.BPs[i].GlobalBPEnable == false);
  }

  REQUIRE(NumExceptions == 3);
}

// Testing of data breakpoints.
void test7(size_t Size, bool Read, bool ExpectHit, size_t offset = 0) {
  NumExceptions = 0;
  BreakpointAssignments bp {};
  bp.BPs[0].Addr = reinterpret_cast<size_t>(&FaultData) + offset;
  bp.BPs[0].GlobalBPEnable = false;
  bp.BPs[0].LocalBPEnable = true;

  switch (Size) {
  case 1: bp.BPs[0].Length = LengthClass::Byte; break;
  case 2: bp.BPs[0].Length = LengthClass::Word; break;
  case 4: bp.BPs[0].Length = LengthClass::DWord; break;
  case 8: bp.BPs[0].Length = LengthClass::QWord; break;
  default: assert(false && "Unhandled size");
  }

  bp.BPs[0].TypeOfFault = Read ? TypeClass::DataReadWrite : TypeClass::DataWrite;

  // Write a breakpoint.
  WriteBreakpoints(&bp);

  size_t Result = ~0U;
  if (!Read) {
    FaultData.u8 = 1;
    Result = 1;
  } else {
    Result = FaultData.u8;
  }

  memset(&bp, 0x42, sizeof(bp));

  // Read it back.
  ReadBreakpoints(&bp);

  constexpr uint32_t MeaningfulBits = (0b111 << 13) | (1 << 1) | (0xF);
  bp.DebugStatusRegister &= MeaningfulBits;

  REQUIRE(FaultData.u8 == Result);

  // Only bp0 fired.
  CHECK((bp.DebugStatusRegister & MeaningfulBits) == (ExpectHit ? (1 << 0) : 0));

  // A bit complex for how many exceptions are expected.
  uint32_t ExpectedHits = 2;
  if (!Read && ExpectHit) {
    ExpectedHits = 3;
  } else if (Read && ExpectHit) {
    ExpectedHits = 4;
  }
  REQUIRE(NumExceptions == ExpectedHits);
}

TEST_CASE("Test Harness") {
  InstallSignalHandler();

  test0();
  test1();
  test2();
  test3();
  test4();
  for (size_t i = 0; i < 4; ++i) {
    test5(i, false);
    test5(i, true);
  }
  for (size_t i = 0; i < 4; ++i) {
    test6(i, false);
    test6(i, true);
  }

  // FEX-Emu doesn't currently support data breakpoints.
  if constexpr (false) {
    struct Test7Data {
      size_t Size;
      bool Read;
      bool ExpectHit;
      size_t Offset;
    };
    Test7Data Test7[] = {
      // 1-byte.
      {1, false, true, 7},
      {1, true, true, 7},
      {1, false, false, 6},
      {1, true, false, 6},
      // 2-byte
      {2, false, true, 6},
      {2, true, true, 6},
      {2, false, false, 4},
      {2, true, false, 4},
      // 4-byte
      {4, false, true, 4},
      {4, true, true, 4},
      {4, false, false, 0},
      {4, true, false, 0},
#ifdef __x86_64__
      // 8-byte
      {8, false, true, 0},
      {8, true, true, 0},
#endif
    };

    for (auto Data : Test7) {
      test7(Data.Size, Data.Read, Data.ExpectHit, Data.Offset);
    }
  }
}
