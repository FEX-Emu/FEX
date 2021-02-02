#pragma once
#include <cstddef>
#include <cstdint>

namespace FEXCore {
  namespace x86_64 {
    // uc_flags flags
    ///< Has extended FP state
    constexpr uint64_t UC_FP_XSTATE         = (1ULL << 0);
    ///< Set when kernel saves SS register from 64bit code
    constexpr uint64_t UC_SIGCONTEXT_SS     = (1ULL << 1);
    ///< Set when kernel will strictly restore the SS
    constexpr uint64_t UC_STRICT_RESTORE_SS = (1ULL << 2);

    ///< Describes the signal stack
    struct __attribute__((packed)) stack_t {
      void *ss_sp;
      int32_t ss_flags;
      uint32_t : 32;
      size_t ss_size;
    };
    static_assert(sizeof(FEXCore::x86_64::stack_t) == 24, "This needs to be the right size");

    struct __attribute__((packed)) _libc_fpstate {
      // This is in FXSAVE format
      uint16_t fcw;
      uint16_t fsw;
      uint16_t ftw;
      uint16_t fop;
      uint64_t fip;
      uint64_t fdp;
      uint32_t mxcsr;
      uint32_t mxcsr_mask;
      __uint128_t _st[8];
      __uint128_t _xmm[16];
      uint32_t _res[24];
    };
    static_assert(sizeof(FEXCore::x86_64::_libc_fpstate) == 512, "This needs to be the right size");

    ///< The order of these must match the GNU ordering
    enum ContextRegs {
      FEX_REG_R8 = 0,
      FEX_REG_R9,
      FEX_REG_R10,
      FEX_REG_R11,
      FEX_REG_R12,
      FEX_REG_R13,
      FEX_REG_R14,
      FEX_REG_R15,
      FEX_REG_RDI,
      FEX_REG_RSI,
      FEX_REG_RBP,
      FEX_REG_RBX,
      FEX_REG_RDX,
      FEX_REG_RAX,
      FEX_REG_RCX,
      FEX_REG_RSP,
      FEX_REG_RIP,
      FEX_REG_EFL,
      FEX_REG_CSGSFS,
      FEX_REG_ERR,
      FEX_REG_TRAPNO,
      FEX_REG_OLDMASK,
      FEX_REG_CR2,
    };
    static_assert(FEX_REG_CR2 == 22, "Oops");

    struct __attribute__((packed)) mcontext_t {
      uint64_t gregs[23];
      FEXCore::x86_64::_libc_fpstate *fpregs;
      uint64_t __reserved[8];
    };
    static_assert(sizeof(FEXCore::x86_64::mcontext_t) == 256, "This needs to be the right size");

    struct __attribute__((packed)) sigset_t {
      uint64_t val[16];
    };
    static_assert(sizeof(FEXCore::x86_64::sigset_t) == 128, "This needs to be the right size");

    struct __attribute__((packed)) ucontext_t {
      uint64_t uc_flags;
      FEXCore::x86_64::ucontext_t *uc_link;
      FEXCore::x86_64::stack_t uc_stack;
      FEXCore::x86_64::mcontext_t uc_mcontext;
      FEXCore::x86_64::sigset_t uc_sigmask;
      FEXCore::x86_64::_libc_fpstate __fpregs_mem;
      uint64_t __ssp[4];
    };
    static_assert(offsetof(FEXCore::x86_64::ucontext_t, uc_mcontext) == 40, "Needs to be correct");

    static_assert(sizeof(FEXCore::x86_64::ucontext_t) == 968, "This needs to be the right size");
  }

  namespace x86 {
    struct __attribute__((packed)) siginfo_t {
      uint32_t pad[32];
    };
    static_assert(sizeof(FEXCore::x86::siginfo_t) == 128, "This needs to be the right size");

    struct __attribute__((packed)) ucontext_t {
      uint32_t pad[91];
    };
    static_assert(sizeof(FEXCore::x86::ucontext_t) == 364, "This needs to be the right size");

  }
}
