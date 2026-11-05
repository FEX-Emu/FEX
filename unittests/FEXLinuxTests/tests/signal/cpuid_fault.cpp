#include <catch2/catch_test_macros.hpp>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <signal.h>
#include <unistd.h>

#ifndef ARCH_GET_CPUID
#define ARCH_GET_CPUID 0x1011
#endif
#ifndef ARCH_SET_CPUID
#define ARCH_SET_CPUID 0x1012
#endif

struct cpuid_fn {
  uint32_t eax, ebx, ecx, edx;
};

static cpuid_fn get_cpuid(uint32_t func, uint32_t leaf = 0) {
  cpuid_fn fn {};
  __asm volatile("cpuid" : "=a"(fn.eax), "=b"(fn.ebx), "=c"(fn.ecx), "=d"(fn.edx) : "a"(func), "c"(leaf));

  return fn;
}

static cpuid_fn get_looooong_cpuid(uint32_t func, uint32_t leaf = 0) {
  cpuid_fn fn {};
  __asm volatile(".byte 0x66, 0x66, 0x66, 0x66;"
                 ".byte 0x66, 0x66, 0x66, 0x66;"
                 ".byte 0x66, 0x66, 0x66, 0x66;"
                 ".byte 0x66;"
                 "cpuid"
                 : "=a"(fn.eax), "=b"(fn.ebx), "=c"(fn.ecx), "=d"(fn.edx)
                 : "a"(func), "c"(leaf));

  return fn;
}

static cpuid_fn get_cpuid_2(uint32_t func, uint32_t leaf = 0) {
  cpuid_fn fn {};
  __asm volatile(".byte 0x26;"
                 "cpuid"
                 : "=a"(fn.eax), "=b"(fn.ebx), "=c"(fn.ecx), "=d"(fn.edx)
                 : "a"(func), "c"(leaf));

  return fn;
}

static cpuid_fn get_cpuid_3(uint32_t func, uint32_t leaf = 0) {
  cpuid_fn fn {};
  __asm volatile(".byte 0x2e;"
                 "cpuid"
                 : "=a"(fn.eax), "=b"(fn.ebx), "=c"(fn.ecx), "=d"(fn.edx)
                 : "a"(func), "c"(leaf));

  return fn;
}

static cpuid_fn get_cpuid_4(uint32_t func, uint32_t leaf = 0) {
  cpuid_fn fn {};
  __asm volatile(".byte 0x36;"
                 "cpuid"
                 : "=a"(fn.eax), "=b"(fn.ebx), "=c"(fn.ecx), "=d"(fn.edx)
                 : "a"(func), "c"(leaf));

  return fn;
}

static cpuid_fn get_cpuid_5(uint32_t func, uint32_t leaf = 0) {
  cpuid_fn fn {};
  __asm volatile(".byte 0x3E;"
                 "cpuid"
                 : "=a"(fn.eax), "=b"(fn.ebx), "=c"(fn.ecx), "=d"(fn.edx)
                 : "a"(func), "c"(leaf));

  return fn;
}

static cpuid_fn get_cpuid_6(uint32_t func, uint32_t leaf = 0) {
  cpuid_fn fn {};
  __asm volatile(".byte 0x64;"
                 "cpuid"
                 : "=a"(fn.eax), "=b"(fn.ebx), "=c"(fn.ecx), "=d"(fn.edx)
                 : "a"(func), "c"(leaf));

  return fn;
}

static cpuid_fn get_cpuid_7(uint32_t func, uint32_t leaf = 0) {
  cpuid_fn fn {};
  __asm volatile(".byte 0x65;"
                 "cpuid"
                 : "=a"(fn.eax), "=b"(fn.ebx), "=c"(fn.ecx), "=d"(fn.edx)
                 : "a"(func), "c"(leaf));

  return fn;
}

static cpuid_fn get_cpuid_8(uint32_t func, uint32_t leaf = 0) {
  cpuid_fn fn {};
  __asm volatile(".byte 0x66;"
                 "cpuid"
                 : "=a"(fn.eax), "=b"(fn.ebx), "=c"(fn.ecx), "=d"(fn.edx)
                 : "a"(func), "c"(leaf));

  return fn;
}

static cpuid_fn get_cpuid_9(uint32_t func, uint32_t leaf = 0) {
  cpuid_fn fn {};
  __asm volatile(".byte 0x67;"
                 "cpuid"
                 : "=a"(fn.eax), "=b"(fn.ebx), "=c"(fn.ecx), "=d"(fn.edx)
                 : "a"(func), "c"(leaf));

  return fn;
}

static cpuid_fn get_cpuid_10(uint32_t func, uint32_t leaf = 0) {
  cpuid_fn fn {};
  __asm volatile(".byte 0xF2;"
                 "cpuid"
                 : "=a"(fn.eax), "=b"(fn.ebx), "=c"(fn.ecx), "=d"(fn.edx)
                 : "a"(func), "c"(leaf));

  return fn;
}

static cpuid_fn get_cpuid_11(uint32_t func, uint32_t leaf = 0) {
  cpuid_fn fn {};
  __asm volatile(".byte 0xf3;"
                 "cpuid"
                 : "=a"(fn.eax), "=b"(fn.ebx), "=c"(fn.ecx), "=d"(fn.edx)
                 : "a"(func), "c"(leaf));

  return fn;
}

#ifdef REG_RIP
// 0x40 isn't REX prefix on 32-bit.
static cpuid_fn get_cpuid_12(uint32_t func, uint32_t leaf = 0) {
  cpuid_fn fn {};
  __asm volatile(".byte 0x40;"
                 "cpuid"
                 : "=a"(fn.eax), "=b"(fn.ebx), "=c"(fn.ecx), "=d"(fn.edx)
                 : "a"(func), "c"(leaf));

  return fn;
}
#endif

static size_t is_cpuid(uint8_t* data) {
  size_t inst_size {};

  while (true) {
    if (inst_size == 15) {
      return 0;
    }
    uint8_t byte = data[inst_size];

    switch (byte) {
    case 0x26: // ES
    case 0x2e: // CS
    case 0x36: // SS
    case 0x3E: // DS
    case 0x64: // FS
    case 0x65: // GS
    case 0x66: // Opcode size
    case 0x67: // Addr size
    case 0xF2: // REPNE
    case 0xF3: // REPE
#ifdef REG_RIP
    case 0x40 ... 0x4f: // REX
#endif
      break;

    case 0xF0: return 0; // Lock explicitly isn't supported on CPUID, should fault.
    case 0xf: {
      ++inst_size;
      byte = data[inst_size];
      if (byte == 0xa2) {
        return inst_size + 1;
      }

      // Nope
      return 0;
    }
    }
    ++inst_size;
  }
  return inst_size;
}

static int faults {};
static void SignalHandler(int signal, siginfo_t* siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;

#ifdef REG_RIP
#define FEX_IP_REG REG_RIP
#define FEX_RAX_REG REG_RAX
#define FEX_RBX_REG REG_RBX
#define FEX_RCX_REG REG_RCX
#define FEX_RDX_REG REG_RDX
#else
#define FEX_IP_REG REG_EIP
#define FEX_RAX_REG REG_EAX
#define FEX_RBX_REG REG_EBX
#define FEX_RCX_REG REG_ECX
#define FEX_RDX_REG REG_EDX
#endif
  if (signal != SIGSEGV) {
    FAIL("Incorrect signal");
  }

  if (_context->uc_mcontext.gregs[REG_TRAPNO] != 13) {
    FAIL("Incorrect trapno");
  }

  if (_context->uc_mcontext.gregs[REG_ERR] != 0) {
    FAIL("Incorrect reg_err");
  }

  if (siginfo->si_code != 0x80) {
    FAIL("Incorrect si_code");
  }

  size_t inst_size = is_cpuid(reinterpret_cast<uint8_t*>(_context->uc_mcontext.gregs[FEX_IP_REG]));
  if (inst_size == 0) {
    FAIL("Isn't cpuid");
  }

  // Return bogus data
  _context->uc_mcontext.gregs[FEX_RAX_REG] = 0x41424344U + faults;
  _context->uc_mcontext.gregs[FEX_RBX_REG] = 0x51525354U + faults;
  _context->uc_mcontext.gregs[FEX_RCX_REG] = 0x61626364U + faults;
  _context->uc_mcontext.gregs[FEX_RDX_REG] = 0x71727374U + faults;

  _context->uc_mcontext.gregs[FEX_IP_REG] += inst_size;

  ++faults;
}

TEST_CASE("cpuid faulting") {
  cpuid_fn fn0 = get_cpuid(0);
  if (syscall(SYS_arch_prctl, ARCH_SET_CPUID, (void*)1) != 0) {
    // Unsupported, just skip.
    return;
  }

  struct sigaction act {};
  act.sa_sigaction = SignalHandler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);

  using ptr_type = cpuid_fn (*)(uint32_t, uint32_t);
  ptr_type test_funcs[] = {
    get_cpuid,          get_cpuid_2, get_cpuid_3, get_cpuid_4,  get_cpuid_5,  get_cpuid_6,
    get_cpuid_7,        get_cpuid_8, get_cpuid_9, get_cpuid_10, get_cpuid_11,
#ifdef REG_RIP
    get_cpuid_12,
#endif
    get_looooong_cpuid,
  };

  // Just attempt regular CPUID for now.
  for (auto ptr : test_funcs) {
    faults = 0;
    cpuid_fn fn_emu = ptr(0, 0);
    REQUIRE(faults == 0);

    CHECK(fn0.eax == fn_emu.eax);
    CHECK(fn0.ebx == fn_emu.ebx);
    CHECK(fn0.ecx == fn_emu.ecx);
    CHECK(fn0.edx == fn_emu.edx);
  }

  // Enable faulting.
  REQUIRE(syscall(SYS_arch_prctl, ARCH_SET_CPUID, (void*)0) == 0);

  for (auto ptr : test_funcs) {
    faults = 0;
    cpuid_fn fn_emu = ptr(0, 0);
    REQUIRE(faults == 1);

    CHECK(fn0.eax != fn_emu.eax);
    CHECK(fn0.ebx != fn_emu.ebx);
    CHECK(fn0.ecx != fn_emu.ecx);
    CHECK(fn0.edx != fn_emu.edx);

    CHECK(fn_emu.eax == 0x41424344U);
    CHECK(fn_emu.ebx == 0x51525354U);
    CHECK(fn_emu.ecx == 0x61626364U);
    CHECK(fn_emu.edx == 0x71727374U);
  }

  // Disable faulting.
  REQUIRE(syscall(SYS_arch_prctl, ARCH_SET_CPUID, (void*)1) == 0);

  // Just attempt regular CPUID for now.
  for (auto ptr : test_funcs) {
    faults = 0;
    cpuid_fn fn_emu = ptr(0, 0);
    REQUIRE(faults == 0);

    CHECK(fn0.eax == fn_emu.eax);
    CHECK(fn0.ebx == fn_emu.ebx);
    CHECK(fn0.ecx == fn_emu.ecx);
    CHECK(fn0.edx == fn_emu.edx);
  }
}
