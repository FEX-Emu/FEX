#include <catch2/catch.hpp>
#include <csetjmp>
#include <unistd.h>
#include <sys/syscall.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

struct CPUState {
  uint64_t Registers[16];

  uint64_t eflags;
};

CPUState CapturedState{};

// This refers to the label defined in the inline ASM below.
extern "C" uint64_t HLT_INST;
void * const HaltLocation = &HLT_INST;

__attribute__((naked))
void DoZeroRegSyscallFault(CPUState *State) {
  // x86-64 ABI puts State pointer in to RDI
  __asm volatile(R"(
    // Save some registers
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    // Load flags
    push qword [rdi + %[FlagsOffset]]
    popfq

    // Do getpid syscall.
    // Overwrites some arguments.
    // Syscall num
    mov rax, qword [rdi + %[RAXOffset]]

    // Load remaining registers that we can
    mov rbx, qword [rdi + %[RBXOffset]];
    mov rcx, qword [rdi + %[RCXOffset]];
    mov rdx, qword [rdi + %[RDXOffset]]
    mov rsi, qword [rdi + %[RSIOffset]]
    mov rbp, qword [rdi + %[RBPOffset]];
    // Can't load RSP
    mov r8, qword [rdi + %[R8Offset]]
    mov r9, qword [rdi + %[R9Offset]];
    mov r10, qword [rdi + %[R10Offset]]
    mov r11, qword [rdi + %[R11Offset]];
    mov r12, qword [rdi + %[R12Offset]];
    mov r13, qword [rdi + %[R13Offset]];
    mov r14, qword [rdi + %[R14Offset]];
    mov r15, qword [rdi + %[R15Offset]];

    // Overwrite RDI last.
    mov rdi, qword [rdi + %[RDIOffset]];

    syscall;

    // Immediately fault
    HLT_INST:
    hlt;

    // We long jump from the signal handler, so this won't continue.
  )"
  :
  // integers are offset by 8 for some reason.
  : [RAXOffset] "i" (offsetof(CPUState, Registers[REG_RAX]) - 8)
  , [RDXOffset] "i" (offsetof(CPUState, Registers[REG_RDX]) - 8)
  , [R10Offset] "i" (offsetof(CPUState, Registers[REG_R10]) - 8)
  , [R8Offset] "i" (offsetof(CPUState, Registers[REG_R8]) - 8)
  , [RSIOffset] "i" (offsetof(CPUState, Registers[REG_RSI]) - 8)
  , [RDIOffset] "i" (offsetof(CPUState, Registers[REG_RDI]) - 8)
  , [RBXOffset] "i" (offsetof(CPUState, Registers[REG_RBX]) - 8)
  , [RCXOffset] "i" (offsetof(CPUState, Registers[REG_RCX]) - 8)
  , [RBPOffset] "i" (offsetof(CPUState, Registers[REG_RBP]) - 8)
  , [R9Offset] "i" (offsetof(CPUState, Registers[REG_R9]) - 8)
  , [R11Offset] "i" (offsetof(CPUState, Registers[REG_R11]) - 8)
  , [R12Offset] "i" (offsetof(CPUState, Registers[REG_R12]) - 8)
  , [R13Offset] "i" (offsetof(CPUState, Registers[REG_R13]) - 8)
  , [R14Offset] "i" (offsetof(CPUState, Registers[REG_R14]) - 8)
  , [R15Offset] "i" (offsetof(CPUState, Registers[REG_R15]) - 8)
  , [FlagsOffset] "i" (offsetof(CPUState, eflags) - 8)

  : "memory");
}


static jmp_buf LongJump{};
static void CapturingHandler(int signal, siginfo_t *siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;

  auto RAX = _context->uc_mcontext.gregs[REG_RAX];
  if (RAX > -4095U) {
    // Failure to syscall
    fprintf(stderr, "Parent thread failed to syscall: %ld %s\n", static_cast<uint64_t>(-RAX), strerror(-RAX));
    _exit(1);
  }

  CPUState &State = CapturedState;

  memcpy(&State.Registers, _context->uc_mcontext.gregs, sizeof(State.Registers));

  longjmp(LongJump, 1);
}


TEST_CASE("getppid: State") {
  // Set up a signal handler for SIGSEGV
  struct sigaction act{};
  act.sa_sigaction = CapturingHandler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);

  CPUState Object = {
    .Registers = {
      0x1011'1213'1415'1617ULL,
      0x2022'2223'2425'2627ULL,
      0x3033'3233'3435'3637ULL,
      0x4044'4243'4445'4647ULL,
      0x5055'5253'5455'5657ULL,
      0x6066'6263'6465'6667ULL,
      0x7077'7273'7475'7677ULL,
      0x8088'8283'8485'8687ULL,
      0x9099'9293'9495'9697ULL,
      0xA0AA'A2A3'A4A5'A6A7ULL,
      0xB0BB'B2B3'B4B5'B6B7ULL,
      0xC0CC'C2C3'C4C5'C6C7ULL,
      0xD0DD'D2D3'D4D5'D6D7ULL,
      0xE0EE'E2E3'E4E5'E6E7ULL,
      0xF0FF'F2F3'F4F5'F6F7ULL,
      0x0000'0203'0405'0607ULL,
    },
    .eflags =
      (1U << 0) | // CF
      (1U << 1) | // RA1
      (1U << 2) | // PF
      (1U << 4) | // AF
      (1U << 6) | // ZF
      (1U << 7) | // CF
      (1U << 9) | // IF (Is always 1 in userspace)
      (1U << 11) // OF
  };

  constexpr uint64_t SyscallNum = SYS_sched_yield;
  Object.Registers[REG_RAX] = SyscallNum;
  int Value = setjmp(LongJump);
  if (Value == 0) {
    DoZeroRegSyscallFault(&Object);
  }

  for (size_t i = 0; i < 16; ++i) {
    if (i == REG_R11 || i == REG_RCX || i == REG_RSP || i == REG_RAX) {
      // Needs special handling.
      continue;
    }

    CHECK(Object.Registers[i] == CapturedState.Registers[i]);
  }

  // Syscall success return
  CHECK(CapturedState.Registers[REG_RAX] == 0);

  // syscall instruction RCX return.
  CHECK(CapturedState.Registers[REG_RCX] == reinterpret_cast<uint64_t>(HaltLocation));

  // Syscall instruction R11 eflags return.
  CHECK(Object.eflags == CapturedState.Registers[REG_R11]);

  // RSP is untested here.

}
