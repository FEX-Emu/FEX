#include <catch2/catch_test_macros.hpp>
#include <csetjmp>
#include <unistd.h>
#include <sys/syscall.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

struct CPUState {
  uint32_t Registers[8];
  uint32_t eflags;
};

CPUState CapturedState {};

enum RegNums {
  TEST_REG_EAX = 0,
  TEST_REG_EBX,
  TEST_REG_ECX,
  TEST_REG_EDX,
  TEST_REG_ESI,
  TEST_REG_EDI,
  TEST_REG_ESP,
  TEST_REG_EBP,
};

__attribute__((naked)) void DoZeroRegSyscallFault(CPUState State) {
  // i386 stores arguments on the stack.
  __asm volatile(
    R"(
    // Load flags
    push dword ptr [esp + %[FlagsOffset]]
    popfd

    // Do getpid syscall.
    // Overwrites some arguments.
    // Syscall num
    mov eax, dword ptr [esp + %[RAXOffset]]

    // Load remaining registers that we can
    mov ebx, dword ptr [esp + %[RBXOffset]];
    mov ecx, dword ptr [esp + %[RCXOffset]];
    mov edx, dword ptr [esp + %[RDXOffset]]
    mov esi, dword ptr [esp + %[RSIOffset]]
    mov edi, dword ptr [esp + %[RDIOffset]];
    mov ebp, dword ptr [esp + %[RBPOffset]];
    // Can't load RSP

    int 0x80;

    // Immediately fault
    hlt;

    // We long jump from the signal handler, so this won't continue.
  )"
    :
    // The stack is offset by 4-bytes due to the call.
    : [RAXOffset] "i"(offsetof(CPUState, Registers[TEST_REG_EAX]) + 4), [RDXOffset] "i"(offsetof(CPUState, Registers[TEST_REG_EDX]) + 4),
      [RSIOffset] "i"(offsetof(CPUState, Registers[TEST_REG_ESI]) + 4), [RDIOffset] "i"(offsetof(CPUState, Registers[TEST_REG_EDI]) + 4),
      [RBXOffset] "i"(offsetof(CPUState, Registers[TEST_REG_EBX]) + 4), [RCXOffset] "i"(offsetof(CPUState, Registers[TEST_REG_ECX]) + 4),
      [RBPOffset] "i"(offsetof(CPUState, Registers[TEST_REG_EBP]) + 4), [FlagsOffset] "i"(offsetof(CPUState, eflags) + 4)

    : "memory");
}

static jmp_buf LongJump {};
static void CapturingHandler(int signal, siginfo_t* siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;

  auto RAX = _context->uc_mcontext.gregs[REG_EAX];
  if (RAX > -4095U) {
    // Failure to syscall
    fprintf(stderr, "Parent thread failed to syscall: %d %s\n", static_cast<uint32_t>(-RAX), strerror(-RAX));
    _exit(1);
  }

  CPUState& State = CapturedState;

  // These aren't 1:1 mapped
#define COPY(REG) State.Registers[TEST_REG_##REG] = _context->uc_mcontext.gregs[REG_##REG];
  COPY(EAX);
  COPY(EBX);
  COPY(ECX);
  COPY(EDX);
  COPY(ESI);
  COPY(EDI);
  COPY(ESP);
  COPY(EBP);

  longjmp(LongJump, 1);
}


TEST_CASE("getppid: State") {
  // Set up a signal handler for SIGSEGV
  struct sigaction act {};
  act.sa_sigaction = CapturingHandler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);

  CPUState Object = {
    .Registers =
      {
        0x1011'1213ULL,
        0x2022'2223ULL,
        0x3033'3233ULL,
        0x4044'4243ULL,
        0x5055'5253ULL,
        0x6066'6263ULL,
        0x7077'7273ULL,
        0x8088'8283ULL,
      },
    .eflags = (1U << 0) | // CF
              (1U << 1) | // RA1
              (1U << 2) | // PF
              (1U << 4) | // AF
              (1U << 6) | // ZF
              (1U << 7) | // CF
              (1U << 9) | // IF (Is always 1 in userspace)
              (1U << 11)  // OF
  };

  constexpr uint64_t SyscallNum = SYS_sched_yield;
  Object.Registers[TEST_REG_EAX] = SyscallNum;
  int Value = setjmp(LongJump);
  if (Value == 0) {
    DoZeroRegSyscallFault(Object);
  }

  for (size_t i = 0; i < (sizeof(Object.Registers) / sizeof(Object.Registers[0])); ++i) {
    if (i == TEST_REG_ESP || i == TEST_REG_EAX) {
      // Needs special handling.
      continue;
    }

    CHECK(Object.Registers[i] == CapturedState.Registers[i]);
  }

  // Syscall success return
  CHECK(CapturedState.Registers[TEST_REG_EAX] == 0);
  // RSP is untested here.
}
