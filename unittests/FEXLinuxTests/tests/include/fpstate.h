#pragma once
#include <cstdint>

namespace FEX::Unittests {
#ifndef __x86_64__
struct __uint128_t {
  uint64_t raw[2];
};
#endif
struct fpx_sw_bytes {
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

struct xstate_header {
  uint64_t xfeatures;
  uint64_t reserved1[2];
  uint64_t reserved2[5];
};
static_assert(sizeof(xstate_header) == 64);

struct ymmh_state {
  __uint128_t ymmh_space[16];
};
static_assert(sizeof(ymmh_state) == 256);

#ifdef __x86_64__
struct _libc_fpstate {
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
static_assert(sizeof(FEX::Unittests::_libc_fpstate) == 512, "This needs to be the right size");

/**
 * Extended state that includes both the main fpstate
 * and the extended state.
 */
struct xstate {
  _libc_fpstate fpstate;
  xstate_header xstate_hdr;
  ymmh_state ymmh;
};
static_assert(sizeof(xstate) == 832);
#else

struct _libc_fpreg {
  uint16_t significand[4];
  uint16_t exponent;
};
static_assert(sizeof(FEX::Unittests::_libc_fpreg) == 10, "This needs to be the right size");

enum fpstate_magic {
  // Legacy fpstate
  MAGIC_FPU = 0xFFFF'0000,
  // Contains extended state information
  MAGIC_XFPSTATE = 0x0,
};
struct _libc_fpstate {
  uint32_t fcw;
  uint32_t fsw;
  uint32_t ftw;
  uint32_t fop;
  uint32_t cssel;
  uint32_t dataoff;
  uint32_t datasel;
  FEX::Unittests::_libc_fpreg _st[8];
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
static_assert(sizeof(FEX::Unittests::_libc_fpstate) == 624, "This needs to be the right size");

struct xstate {
  _libc_fpstate fpstate;
  xstate_header xstate_hdr;
  ymmh_state ymmh;
};
static_assert(sizeof(xstate) == 944);
#endif
} // namespace FEX::Unittests
