// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <signal.h>

namespace FEXCore {
namespace x86_64 {
  // uc_flags flags
  ///< Has extended FP state
  constexpr uint64_t UC_FP_XSTATE = (1ULL << 0);
  ///< Set when kernel saves SS register from 64bit code
  constexpr uint64_t UC_SIGCONTEXT_SS = (1ULL << 1);
  ///< Set when kernel will strictly restore the SS
  constexpr uint64_t UC_STRICT_RESTORE_SS = (1ULL << 2);

  ///< Describes the signal stack
  struct FEX_PACKED stack_t {
    void* ss_sp;
    int32_t ss_flags;
    uint32_t : 32;
    size_t ss_size;
  };
  static_assert(sizeof(FEXCore::x86_64::stack_t) == 24, "This needs to be the right size");

  /**
   * Describes the software specific bytes added at the end of the
   * fpstate to identify whether or not an extended context area is
   * present and what kind of extended features are present in said
   * context area.
   */
  struct FEX_PACKED fpx_sw_bytes {
    static constexpr uint32_t FP_XSTATE_MAGIC_1 = 0x46505853;
    static constexpr uint32_t FP_XSTATE_MAGIC_2 = 0x46505845;

    enum FeatureFlag : uint32_t {
      FEATURE_FP = 1U << 0,
      FEATURE_SSE = 1U << 1,
      FEATURE_YMM = 1U << 2,
      FEATURE_BNDREGS = 1U << 3,
      FEATURE_BNDCSR = 1U << 4,
      FEATURE_OPMASK = 1U << 5,
      FEATURE_ZMM_Hi256 = 1U << 6,
      FEATURE_Hi16_ZMM = 1U << 7,
      FEATURE_PT_UNIMPL = 1U << 8,
      FEATURE_PKRU = 1U << 9,
      FEATURE_PASID = 1U << 10,
      FEATURE_RESERVED11 = 1U << 11,
      FEATURE_RESERVED12 = 1U << 12,
      FEATURE_RESERVED13 = 1U << 13,
      FEATURE_RESERVED14 = 1U << 14,
      FEATURE_LBR = 1U << 15,
      FEATURE_RESERVED16 = 1U << 16,
      FEATURE_XTILE_CFG = 1U << 17,
      FEATURE_XTILE_DATA = 1U << 18,
    };

    bool HasExtendedContext() const {
      return magic1 == FP_XSTATE_MAGIC_1;
    }

    bool HasYMMH() const {
      return (xfeatures & FEATURE_YMM) != 0;
    }

    // If magic1 is set to FP_XSTATE_MAGIC_1, then the encompassing
    // frame is an xstate frame. If 0, then it's a legacy frame.
    uint32_t magic1;

    // Total size of the fpstate area
    // - magic1 = 0                 -> sizeof(fpstate)
    // - magic1 = FP_XSTATE_MAGIC_1 -> sizeof(xstate) + extensions (if any)
    uint32_t extended_size;

    // Feature bitmask describing supported features.
    uint64_t xfeatures;

    // Actual XSAVE state size, based on above xfeatures
    uint32_t xstate_size;

    // Reserved data
    uint32_t padding[7];
  };
  static_assert(sizeof(fpx_sw_bytes) == 48);

  struct FEX_PACKED _libc_fpstate {
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
    uint32_t _res[12];

    // Linux uses 12 of the bytes relegated for software purposes
    // to store info describing any existing XSAVE context data.
    fpx_sw_bytes sw_reserved;
  };
  static_assert(sizeof(FEXCore::x86_64::_libc_fpstate) == 512, "This needs to be the right size");

  struct FEX_PACKED xstate_header {
    uint64_t xfeatures;
    uint64_t reserved1[2];
    uint64_t reserved2[5];
  };
  static_assert(sizeof(xstate_header) == 64);

  struct FEX_PACKED ymmh_state {
    __uint128_t ymmh_space[16];
  };
  static_assert(sizeof(ymmh_state) == 256);

  struct FEX_PACKED magic2 {
    uint32_t pad;
    uint32_t magic;
  };
  static_assert(sizeof(magic2) == sizeof(uint64_t));

  /**
   * Extended state that includes both the main fpstate
   * and the extended state.
   */
  struct FEX_PACKED xstate {
    _libc_fpstate fpstate;
    xstate_header xstate_hdr;
    ymmh_state ymmh;
    magic2 magic2 {};
  };
  static_assert(sizeof(xstate) == 840);

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

  struct FEX_PACKED mcontext_t {
    uint64_t gregs[23];
    FEXCore::x86_64::_libc_fpstate* fpregs;
    uint64_t __reserved[8];
  };
  static_assert(sizeof(FEXCore::x86_64::mcontext_t) == 256, "This needs to be the right size");

  struct FEX_PACKED sigset_t {
    uint64_t val[16];
  };
  static_assert(sizeof(FEXCore::x86_64::sigset_t) == 128, "This needs to be the right size");

  struct FEX_PACKED ucontext_t {
    uint64_t uc_flags;
    FEXCore::x86_64::ucontext_t* uc_link;
    FEXCore::x86_64::stack_t uc_stack;
    FEXCore::x86_64::mcontext_t uc_mcontext;
    FEXCore::x86_64::sigset_t uc_sigmask;
  };
  static_assert(offsetof(FEXCore::x86_64::ucontext_t, uc_mcontext) == 40, "Needs to be correct");

  static_assert(sizeof(FEXCore::x86_64::ucontext_t) == 424, "This needs to be the right size");
} // namespace x86_64

namespace x86 {
  // uc_flags flags
  ///< Has extended FP state
  constexpr uint64_t UC_FP_XSTATE = (1ULL << 0);

  ///< The order of these must match the GNU ordering
  enum ContextRegs {
    FEX_REG_GS = 0,
    FEX_REG_FS,
    FEX_REG_ES,
    FEX_REG_DS,
    FEX_REG_RDI,
    FEX_REG_RSI,
    FEX_REG_RBP,
    FEX_REG_RSP,
    FEX_REG_RBX,
    FEX_REG_RDX,
    FEX_REG_RCX,
    FEX_REG_RAX,
    FEX_REG_TRAPNO,
    FEX_REG_ERR,
    FEX_REG_EIP,
    FEX_REG_CS,
    FEX_REG_EFL,
    FEX_REG_UESP,
    FEX_REG_SS
  };
  static_assert(FEX_REG_SS == 18, "Oops");

  union sigval_t {
    int sival_int;
    uint32_t sival_ptr; // XXX: Should be compat_ptr<void>
  };

  struct FEX_PACKED siginfo_t {
    int si_signo;
    int si_errno;
    int si_code;
    union {
      uint32_t pad[29];
      /* tgkill siginfo_t */
      struct {
        int32_t pid;
        int32_t uid;
      } _kill;
      /* SIGPOLL */
      struct {
        int32_t band;
        int32_t fd;
      } _poll;
      /* SIGILL, SIGFPE, SIGSEGV, SIBUS */
      struct {
        uint32_t addr;
      } _sigfault;
      /* SIGCHLD */
      struct {
        int32_t pid;
        int32_t uid;
        int32_t status;
        int32_t utime;
        int32_t stime;
      } _sigchld;
      /* RT signals */
      struct {
        int32_t pid;
        int32_t uid;
        union {
          int32_t sival_int;
          uint32_t sival_ptr; // compat_ptr
        } sigval;
      } _rt;
      /* SIGALRM, SIGVTALRM */
      struct {
        int tid;
        int overrun;
        FEXCore::x86::sigval_t sigval;
      } _timer;
      /* SIGSYS */
      struct {
        uint32_t call_addr; // compat_ptr
        int32_t syscall;
        uint32_t arch;
      } _sigsys;
    } _sifields;

    union HostSigInfo_t {
      // This anonymous struct needs to match the host definition
      struct {
        uint32_t si_signo;
        uint32_t si_errno;
        uint32_t si_code;

        uint32_t __pad0;

        // Pad[28] is a union for all the sifields
        uint32_t _pad[28];
      } FEXDef;
      ::siginfo_t host {};
    };
    static_assert(sizeof(HostSigInfo_t) == 128, "This needs to be the right size");

    siginfo_t() = delete;

    operator ::siginfo_t() const {
      // The definition of siginfo_t changes depending on the host environment
      // It is guaranteed to be 128 bytes and the kernel interface is the same for all of them
      // Since we only run on Linux
      HostSigInfo_t val {};

      val.FEXDef.si_signo = si_signo;
      val.FEXDef.si_errno = si_errno;
      val.FEXDef.si_code = si_code;

      // Host siginfo has a pad member that is set to zeros
      val.FEXDef.__pad0 = 0;

      // Copy over the union
      // The union is different sizes on 64-bit versus 32-bit
      memcpy(val.FEXDef._pad, _sifields.pad, std::min(sizeof(val.FEXDef._pad), sizeof(_sifields.pad)));

      return val.host;
    }

    siginfo_t(::siginfo_t val) {
      HostSigInfo_t host;
      host.host = val;

      si_signo = host.FEXDef.si_signo;
      si_errno = host.FEXDef.si_errno;
      si_code = host.FEXDef.si_code;

      // Copy over the union
      // The union is different sizes on 64-bit versus 32-bit
      memcpy(_sifields.pad, host.FEXDef._pad, std::min(sizeof(host.FEXDef._pad), sizeof(_sifields.pad)));
    }
    static_assert(offsetof(::siginfo_t, si_signo) == offsetof(HostSigInfo_t, FEXDef.si_signo), "si_signo in wrong location?");
    static_assert(offsetof(::siginfo_t, si_errno) == offsetof(HostSigInfo_t, FEXDef.si_errno), "si_errno in wrong location?");
    static_assert(offsetof(::siginfo_t, si_code) == offsetof(HostSigInfo_t, FEXDef.si_code), "si_code in wrong location?");
  };
  static_assert(sizeof(FEXCore::x86::siginfo_t) == 128, "This needs to be the right size");

  struct FEX_PACKED stack_t {
    uint32_t ss_sp; // XXX: should be compat_ptr<void>
    int ss_flags;
    uint32_t ss_size;
  };

  static_assert(sizeof(FEXCore::x86::stack_t) == 12, "This needs to be the right size");

  struct FEX_PACKED mcontext_t {
    uint32_t gregs[19];
    uint32_t fpregs; // XXX: should be compat_ptr<FEXCore::x86::_libc_fpstate>
    uint32_t oldmask;
    uint32_t cr2;
  };
  static_assert(sizeof(FEXCore::x86::mcontext_t) == 88, "This needs to be the right size");

  struct _libc_fpreg {
    uint16_t significand[4];
    uint16_t exponent;
  };
  static_assert(sizeof(FEXCore::x86::_libc_fpreg) == 10, "This needs to be the right size");

  // Same layout on both x86 and x86_64
  using fpx_sw_bytes = x86_64::fpx_sw_bytes;
  using xstate_header = x86_64::xstate_header;
  using ymmh_state = x86_64::ymmh_state;

  enum fpstate_magic {
    // Legacy fpstate
    MAGIC_FPU = 0xFFFF'0000,
    // Contains extended state information
    MAGIC_XFPSTATE = 0x0,
  };
  struct FEX_PACKED _libc_fpstate {
    uint32_t fcw;
    uint32_t fsw;
    uint32_t ftw;
    uint32_t fop;
    uint32_t cssel;
    uint32_t dataoff;
    uint32_t datasel;
    FEXCore::x86::_libc_fpreg _st[8];
    uint32_t status;

    // Extended FPU data
    uint32_t pad[6]; // Ignored FXSR data
    uint32_t mxcsr;
    uint32_t reserved;
    __uint128_t _st_pad[8];   // Ignored st data
    __uint128_t _xmm[8];      // First 8 XMM registers
    uint32_t pad2[44];        // Second 8 XMM registers plus padding
    fpx_sw_bytes sw_reserved; // extended state encoding
  };
  static_assert(sizeof(FEXCore::x86::_libc_fpstate) == 624, "This needs to be the right size");

  /**
   * Extended state that includes both the main fpstate
   * and the extended state.
   */
  struct FEX_PACKED xstate {
    _libc_fpstate fpstate;
    xstate_header xstate_hdr;
    ymmh_state ymmh;
    x86_64::magic2 magic2 {};
  };
  static_assert(sizeof(xstate) == 952);

  struct FEX_PACKED ucontext_t {
    uint32_t uc_flags;
    uint32_t uc_link; // XXX: should be a compat_ptr<FEXCore::x86::ucontext_t>
    FEXCore::x86::stack_t uc_stack;
    FEXCore::x86::mcontext_t uc_mcontext;
    FEXCore::x86_64::sigset_t uc_sigmask; // This matches across architectures
  };
  static_assert(sizeof(FEXCore::x86::ucontext_t) == 236, "This needs to be the right size");

  ///< Non-rt signal context.
  //
  // Needs to match the format expected from signal handlers without SA_SIGINFO set.
  struct sigcontext {
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t di;
    uint32_t si;
    uint32_t bp;
    uint32_t sp;
    uint32_t bx;
    uint32_t dx;
    uint32_t cx;
    uint32_t ax;
    uint32_t trapno;
    uint32_t err;
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t sp_at_signal;
    uint32_t ss;

    uint32_t fpstate;
    uint32_t oldmask;
    uint32_t cr2;
  };
} // namespace x86
} // namespace FEXCore
