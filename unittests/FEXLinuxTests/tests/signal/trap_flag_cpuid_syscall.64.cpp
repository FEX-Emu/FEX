#include <catch2/catch_test_macros.hpp>

#include <iterator>
#include <signal.h>
#include <stdint.h>
#include <ucontext.h>

// Single-stepping with TF over CPUID and SYSCALL.
//
// Both instructions leave the JIT block: CPUID runs through a dispatcher stub, SYSCALL through the syscall handler,
// and both return to guest code through an L1 cache lookup. With TF set the trap must still land on the instruction
// after CPUID/SYSCALL and on every instruction that follows.
//
// The lookup only hits when the code behind the instruction is already compiled, so each probe runs from its `Warm`
// label once with TF clear. The stepped run shares nothing else with that first run: it sets TF in a lead-in of its
// own that jumps onto the CPUID/SYSCALL instruction, and the handler clears TF when the trap on the final RET
// arrives. The only compiled block the stepped run can enter is then the one behind CPUID/SYSCALL.

#pragma GCC diagnostic ignored "-Wattributes" // Suppress warning in case control-flow checks aren't enabled

extern "C" void ProbeCPUID();
extern "C" void ProbeCPUIDWarm();
extern "C" void TF_CPUID_1();
extern "C" void TF_CPUID_2();
extern "C" void TF_CPUID_3();
extern "C" void TF_CPUID_4();
extern "C" void TF_CPUID_5();
extern "C" void TF_CPUID_6();
extern "C" void TF_CPUID_7();

// Sets TF. Every instruction after the first one then traps, up to the RET where the handler clears TF.
extern "C" __attribute__((naked, nocf_check)) void ProbeCPUID() {
  __asm volatile(R"(
  pushfq;
  or qword ptr [rsp], 0x100;
  popfq;
  push rbx;
  TF_CPUID_1:
  xor eax, eax;
  TF_CPUID_2:
  jmp TF_CPUID_3;

  ProbeCPUIDWarm:
  push rbx;
  xor eax, eax;
  TF_CPUID_3:
  cpuid;
  TF_CPUID_4:
  nop;
  TF_CPUID_5:
  nop;
  TF_CPUID_6:
  pop rbx;
  TF_CPUID_7:
  ret;
  )");
}

extern "C" void ProbeSYSCALL();
extern "C" void ProbeSYSCALLWarm();
extern "C" void TF_SYSCALL_1();
extern "C" void TF_SYSCALL_2();
extern "C" void TF_SYSCALL_3();
extern "C" void TF_SYSCALL_4();
extern "C" void TF_SYSCALL_5();

extern "C" __attribute__((naked, nocf_check)) void ProbeSYSCALL() {
  __asm volatile(R"(
  pushfq;
  or qword ptr [rsp], 0x100;
  popfq;
  mov eax, 39;
  TF_SYSCALL_1:
  jmp TF_SYSCALL_2;

  ProbeSYSCALLWarm:
  mov eax, 39;
  TF_SYSCALL_2:
  syscall;
  TF_SYSCALL_3:
  nop;
  TF_SYSCALL_4:
  nop;
  TF_SYSCALL_5:
  ret;
  )");
}

constexpr int MAX_TRAPS = 32;
static uintptr_t trap_rips[MAX_TRAPS];
static int trap_count;
static int trap_bad_si_code;
static uintptr_t trap_last_rip;

static void TrapHandler(int, siginfo_t* siginfo, void* context) {
  auto* ctx = static_cast<ucontext_t*>(context);
  const uintptr_t rip = ctx->uc_mcontext.gregs[REG_RIP];
  if (trap_count < MAX_TRAPS) {
    trap_rips[trap_count] = rip;
  }
  trap_count++;
  if (siginfo->si_code != TRAP_TRACE) {
    trap_bad_si_code++;
  }
  // Stop stepping on the probe's RET, or once the traps have run away from the probe.
  if (rip == trap_last_rip || trap_count >= MAX_TRAPS) {
    ctx->uc_mcontext.gregs[REG_EFL] &= ~0x100;
  }
}

static void InstallTrapHandler(void (*last)()) {
  struct sigaction act {};
  act.sa_sigaction = TrapHandler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGTRAP, &act, nullptr);
  trap_count = 0;
  trap_bad_si_code = 0;
  trap_last_rip = reinterpret_cast<uintptr_t>(last);
}

template<size_t N>
static void CheckTraps(const uintptr_t (&expected)[N]) {
  REQUIRE(trap_count == N);
  CHECK(trap_bad_si_code == 0);
  for (size_t i = 0; i < N; ++i) {
    INFO("trap " << i);
    CHECK(trap_rips[i] == expected[i]);
  }
}

TEST_CASE("Signals: Trap Flag across CPUID") {
  ProbeCPUIDWarm();
  InstallTrapHandler(&TF_CPUID_7);
  ProbeCPUID();

  const uintptr_t expected[] = {
    reinterpret_cast<uintptr_t>(&TF_CPUID_1), reinterpret_cast<uintptr_t>(&TF_CPUID_2), reinterpret_cast<uintptr_t>(&TF_CPUID_3),
    reinterpret_cast<uintptr_t>(&TF_CPUID_4), reinterpret_cast<uintptr_t>(&TF_CPUID_5), reinterpret_cast<uintptr_t>(&TF_CPUID_6),
    reinterpret_cast<uintptr_t>(&TF_CPUID_7),
  };
  CheckTraps(expected);
}

TEST_CASE("Signals: Trap Flag across SYSCALL") {
  ProbeSYSCALLWarm();
  InstallTrapHandler(&TF_SYSCALL_5);
  ProbeSYSCALL();

  // Where the trap after SYSCALL lands depends on the kernel's return path. FEX and a FRED kernel trap before the
  // next instruction runs. A kernel returning through IRET does not trap after the IRET itself, so the first trap
  // after the syscall lands after the following instruction.
  const uintptr_t trap_after_syscall[] = {
    reinterpret_cast<uintptr_t>(&TF_SYSCALL_1), reinterpret_cast<uintptr_t>(&TF_SYSCALL_2), reinterpret_cast<uintptr_t>(&TF_SYSCALL_3),
    reinterpret_cast<uintptr_t>(&TF_SYSCALL_4), reinterpret_cast<uintptr_t>(&TF_SYSCALL_5),
  };
  const uintptr_t trap_after_next[] = {
    reinterpret_cast<uintptr_t>(&TF_SYSCALL_1),
    reinterpret_cast<uintptr_t>(&TF_SYSCALL_2),
    reinterpret_cast<uintptr_t>(&TF_SYSCALL_4),
    reinterpret_cast<uintptr_t>(&TF_SYSCALL_5),
  };
  if (trap_count == std::size(trap_after_next)) {
    CheckTraps(trap_after_next);
  } else {
    CheckTraps(trap_after_syscall);
  }
}
