#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <thread>

#pragma GCC diagnostic ignored "-Wattributes"

__attribute__((aligned(16))) constexpr static uint32_t xmm_values[8][4] = {
  {0x00000001, 0x00000002, 0x00000003, 0x00000004}, {0x00000011, 0x00000012, 0x00000013, 0x00000014},
  {0x00000021, 0x00000022, 0x00000023, 0x00000024}, {0x00000031, 0x00000032, 0x00000033, 0x00000034},
  {0x00000041, 0x00000042, 0x00000043, 0x00000044}, {0x00000051, 0x00000052, 0x00000053, 0x00000054},
  {0x00000061, 0x00000062, 0x00000063, 0x00000064}, {0x00000071, 0x00000072, 0x00000073, 0x00000074},
};

// 4 GPR slots (2 used + 2 pad for 16-byte alignment) + 32 XMM slots
static uint32_t results[4 + 32] __attribute__((aligned(16)));
static volatile int alarm_fired = 0;

static std::atomic<uint32_t> test_ready {};

extern "C" void ContinueAfterSignal();

static void SignalHandler(int signal, siginfo_t* siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;
  alarm_fired = 1;

#ifdef REG_RIP
#define FEX_IP_REG REG_RIP
#else
#define FEX_IP_REG REG_EIP
#endif
  _context->uc_mcontext.gregs[FEX_IP_REG] = reinterpret_cast<greg_t>(ContinueAfterSignal);
#undef FEX_IP_REG
}

// Load GPRs (ECX, EDX) and XMM0-XMM7 with known values, keep them
// live in SRA with a reload loop. SIGALRM fires asynchronously, the
// handler modifies EIP, forcing FEX to spill and reload all register
// state via SpillSRA. After returning, store all registers for comparison.
__attribute__((nocf_check)) static void LoadRegsLoopAndStore() {
  __asm volatile(R"(
    movaps xmm0, [%[v]]
    movaps xmm1, [%[v]+16]
    movaps xmm2, [%[v]+32]
    movaps xmm3, [%[v]+48]
    movaps xmm4, [%[v]+64]
    movaps xmm5, [%[v]+80]
    movaps xmm6, [%[v]+96]
    movaps xmm7, [%[v]+112]

  .Lloop:
    mov ecx, 0x89ABCDEF
    mov edx, 0xFEDCBA98

    movaps xmm0, [%[v]]
    movaps xmm1, [%[v]+16]
    movaps xmm2, [%[v]+32]
    movaps xmm3, [%[v]+48]
    movaps xmm4, [%[v]+64]
    movaps xmm5, [%[v]+80]
    movaps xmm6, [%[v]+96]
    movaps xmm7, [%[v]+112]

    mov %[test_ready], ecx
    mov eax, %[alarm]
    test eax, eax
    jz .Lloop

    .global ContinueAfterSignal
  ContinueAfterSignal:
    mov [%[res]], ecx
    mov [%[res]+4], edx

    movaps [%[res]+16], xmm0
    movaps [%[res]+32], xmm1
    movaps [%[res]+48], xmm2
    movaps [%[res]+64], xmm3
    movaps [%[res]+80], xmm4
    movaps [%[res]+96], xmm5
    movaps [%[res]+112], xmm6
    movaps [%[res]+128], xmm7
  )" ::[v] "r"(&xmm_values),
                 [alarm] "m"(alarm_fired), [res] "r"(results), [test_ready] "m"(test_ready)
                 : "memory", "cc", "eax", "ecx", "edx");
}

static void AlarmThread(int thread_to_alarm) {
  while (!test_ready.load())
    ;
  tgkill(::getpid(), thread_to_alarm, SIGALRM);
}

TEST_CASE("Signals: Register state preserved across async signal with SRA") {
  struct sigaction act {};
  act.sa_sigaction = SignalHandler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGALRM, &act, nullptr);

  alarm_fired = 0;
  memset(results, 0, sizeof(results));

  std::thread t(AlarmThread, ::gettid());
  LoadRegsLoopAndStore();

  REQUIRE(alarm_fired == 1);

  CHECK(results[0] == 0x89ABCDEF);
  CHECK(results[1] == 0xFEDCBA98);

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 4; j++) {
      uint32_t val = results[4 + i * 4 + j];
      if (val != xmm_values[i][j]) {
        printf("XMM%d[%d] = 0x%08x, expected 0x%08x\n", i, j, val, xmm_values[i][j]);
      }
      CHECK(val == xmm_values[i][j]);
    }
  }

  t.join();
}
