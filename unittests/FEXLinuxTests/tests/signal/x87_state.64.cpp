#include <catch2/catch_test_macros.hpp>

#include <signal.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <cstdlib>

struct DataStruct {
  uint64_t dual[2];
};
constexpr static DataStruct data[8] = {
  {0x1112131415161718ULL, 0x191A1B1C1D1E1F10ULL}, {0x2122232425262728ULL, 0x292A2B2C2D2E2F20ULL},
  {0x3132333435363738ULL, 0x393A3B3C3D3E3F30ULL}, {0x4142434445464748ULL, 0x494A4B4C4D4E4F40ULL},
  {0x5152535455565758ULL, 0x595A5B5C5D5E5F50ULL}, {0x6162636465666768ULL, 0x696A6B6C6D6E6F60ULL},
  {0x7172737475767778ULL, 0x797A7B7C7D7E7F70ULL}, {0x8182838485868788ULL, 0x898A8B8C8D8E8F80ULL},
};

extern "C" void RetInstruction();
__attribute__((naked, nocf_check)) static void TestFromSignal(const DataStruct* data) {
  __asm volatile(R"(

  finit;
  // Load 8 zeroes to be safe.
  fldz;
  fldz;
  fldz;
  fldz;
  fldz;
  fldz;
  fldz;
  fldz;

  // Empty them
  ffreep st(0);
  ffreep st(0);
  ffreep st(0);
  ffreep st(0);
  ffreep st(0);
  ffreep st(0);
  ffreep st(0);
  ffreep st(0);

  // Now load **7** values. Keeping the last one zero and our stack top not wrapped around.
  fld tbyte ptr [rdi + (0 * 16)];
  fld tbyte ptr [rdi + (1 * 16)];
  fld tbyte ptr [rdi + (2 * 16)];
  fld tbyte ptr [rdi + (3 * 16)];
  fld tbyte ptr [rdi + (4 * 16)];
  fld tbyte ptr [rdi + (5 * 16)];
  fld tbyte ptr [rdi + (6 * 16)];

  hlt;
  RetInstruction:
  ret;
  )" ::
                   : "memory", "cc");
}

extern "C" void RetSetInstruction();
__attribute__((naked, nocf_check)) static void TestSetInSignal(DataStruct* data) {
  __asm volatile(R"(
  finit;
  // Load 8 zeroes to be safe.
  fldz;
  fldz;
  fldz;
  fldz;
  fldz;
  fldz;
  fldz;
  fldz;

  // Empty them
  ffreep st(0);
  ffreep st(0);
  ffreep st(0);
  ffreep st(0);
  ffreep st(0);
  ffreep st(0);
  ffreep st(0);
  ffreep st(0);

  hlt;
  RetSetInstruction:

  // Store values until the status word says nothing is left.
  mov eax, 0;

2:

  fstsw ax;
  and eax, (7 << 11);
  jz 3f;
  fstp tbyte ptr [rdi];
  add rdi, 16;

  jmp 2b

3:

  // Now load **7** values. Keeping the last one zero and our stack top not wrapped around.
  fld tbyte ptr [rdi + (0 * 16)];
  fld tbyte ptr [rdi + (1 * 16)];
  fld tbyte ptr [rdi + (2 * 16)];
  fld tbyte ptr [rdi + (3 * 16)];
  fld tbyte ptr [rdi + (4 * 16)];
  fld tbyte ptr [rdi + (5 * 16)];
  fld tbyte ptr [rdi + (6 * 16)];

  ret;
  )" ::
                   : "memory", "cc");
}

static DataStruct signal_data[8];

static void Correct_Handler(int signal, siginfo_t* siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;
  auto mcontext = &_context->uc_mcontext;

  for (size_t i = 0; i < 8; ++i) {
    memcpy(&signal_data[i], &mcontext->fpregs->_st[i], 10);
  }
#ifdef REG_RIP
#define FEX_IP_REG REG_RIP
#else
#define FEX_IP_REG REG_EIP
#endif

  mcontext->gregs[FEX_IP_REG] = reinterpret_cast<greg_t>(RetInstruction);
}

static void Set_Signal_Handler(int signal, siginfo_t* siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;
  auto mcontext = &_context->uc_mcontext;

  // Set the first seven values
  for (size_t i = 0; i < 8; ++i) {
    memcpy(&mcontext->fpregs->_st[i], &data[i], sizeof(mcontext->fpregs->_st[i]));
  }

  // Adjust the x87 TOP to 1
  mcontext->fpregs->swd = (mcontext->fpregs->swd & ~(3 << 11)) | (1 << 11);
  // Make sure to set the tag words as valid.
  mcontext->fpregs->ftw = 0xFFFE;

#ifdef REG_RIP
#define FEX_IP_REG REG_RIP
#else
#define FEX_IP_REG REG_EIP
#endif

  mcontext->gregs[FEX_IP_REG] = reinterpret_cast<greg_t>(RetSetInstruction);
}

TEST_CASE("Signals: X87 State in handler") {
  struct sigaction act {};
  act.sa_sigaction = Correct_Handler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);
  TestFromSignal(data);

  constexpr static DataStruct test_data[8] = {
    {0x7172737475767778, 0x7f70}, {0x6162636465666768, 0x6f60}, {0x5152535455565758, 0x5f50}, {0x4142434445464748, 0x4f40},
    {0x3132333435363738, 0x3f30}, {0x2122232425262728, 0x2f20}, {0x1112131415161718, 0x1f10}, {0x0, 0x0}};

  for (size_t i = 0; i < 8; ++i) {
    CHECK(memcmp(&test_data[i], &signal_data[i], sizeof(DataStruct)) == 0);
  }
}

TEST_CASE("Signals: X87 State set state in handler") {
  struct sigaction act {};
  act.sa_sigaction = Set_Signal_Handler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);

  DataStruct output_data[8] {};
  TestSetInSignal(output_data);

  constexpr static DataStruct test_data[8] = {
    {0x1112131415161718, 0x1f10}, {0x2122232425262728, 0x2f20}, {0x3132333435363738, 0x3f30}, {0x4142434445464748, 0x4f40},
    {0x5152535455565758, 0x5f50}, {0x6162636465666768, 0x6f60}, {0x7172737475767778, 0x7f70}, {0x0, 0x0}};

  for (size_t i = 0; i < 8; ++i) {
    CHECK(memcmp(&test_data[i], &output_data[i], sizeof(DataStruct)) == 0);
  }
}
