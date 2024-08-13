// SPDX-License-Identifier: MIT
#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/HostFeatures.h>

#include "aarch64/cpu-aarch64.h"

#ifdef _M_X86_64
#define XBYAK64
#define XBYAK_NO_EXCEPTION
#include <FEXCore/fextl/list.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/unordered_set.h>

#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>
#endif

namespace FEX {

// Data Zero Prohibited flag
// 0b0 = ZVA/GVA/GZVA permitted
// 0b1 = ZVA/GVA/GZVA prohibited
[[maybe_unused]] constexpr uint32_t DCZID_DZP_MASK = 0b1'0000;
// Log2 of the blocksize in 32-bit words
[[maybe_unused]] constexpr uint32_t DCZID_BS_MASK = 0b0'1111;

#ifdef _M_ARM_64
[[maybe_unused]]
static uint32_t GetDCZID() {
  uint64_t Result {};
  __asm("mrs %[Res], DCZID_EL0" : [Res] "=r"(Result));
  return Result;
}

static uint32_t GetFPCR() {
  uint64_t Result {};
  __asm("mrs %[Res], FPCR" : [Res] "=r"(Result));
  return Result;
}

static void SetFPCR(uint64_t Value) {
  __asm("msr FPCR, %[Value]" ::[Value] "r"(Value));
}

__attribute__((naked)) static uint64_t ReadSVEVectorLengthInBits() {
  ///< Can't use rdvl instruction directly because compilers will complain that sve/sme is required.
  __asm(R"(
  .word 0x04bf5100 // rdvl x0, #8
  ret;
  )");
}
#else
[[maybe_unused]]
static uint32_t GetDCZID() {
  // Return unsupported
  return DCZID_DZP_MASK;
}

[[maybe_unused]]
static int ReadSVEVectorLengthInBits() {
  // Return unsupported
  return 0;
}
#endif

static void OverrideFeatures(FEXCore::HostFeatures* Features, uint64_t ForceSVEWidth) {
  // Override features if the user has specifically called for it.
  FEX_CONFIG_OPT(HostFeatures, HOSTFEATURES);
  if (!HostFeatures()) {
    // Early exit if no features are overriden.
    return;
  }

#define ENABLE_DISABLE_OPTION(FeatureName, name, enum_name)                                                                        \
  do {                                                                                                                             \
    const bool Disable##name = (HostFeatures() & FEXCore::Config::HostFeatures::DISABLE##enum_name) != 0;                          \
    const bool Enable##name = (HostFeatures() & FEXCore::Config::HostFeatures::ENABLE##enum_name) != 0;                            \
    LogMan::Throw::AFmt(!(Disable##name && Enable##name), "Disabling and Enabling CPU feature (" #name ") is mutually exclusive"); \
    const bool AlreadyEnabled = Features->FeatureName;                                                                             \
    const bool Result = (AlreadyEnabled | Enable##name) & !Disable##name;                                                          \
    Features->FeatureName = Result;                                                                                                \
  } while (0)

#define GET_SINGLE_OPTION(name, enum_name)                                                              \
  const bool Disable##name = (HostFeatures() & FEXCore::Config::HostFeatures::DISABLE##enum_name) != 0; \
  const bool Enable##name = (HostFeatures() & FEXCore::Config::HostFeatures::ENABLE##enum_name) != 0;   \
  LogMan::Throw::AFmt(!(Disable##name && Enable##name), "Disabling and Enabling CPU feature (" #name ") is mutually exclusive");

  ENABLE_DISABLE_OPTION(SupportsAVX, AVX, AVX);
  ENABLE_DISABLE_OPTION(SupportsSVE128, SVE, SVE);
  ENABLE_DISABLE_OPTION(SupportsAFP, AFP, AFP);
  ENABLE_DISABLE_OPTION(SupportsRCPC, LRCPC, LRCPC);
  ENABLE_DISABLE_OPTION(SupportsTSOImm9, LRCPC2, LRCPC2);
  ENABLE_DISABLE_OPTION(SupportsCSSC, CSSC, CSSC);
  ENABLE_DISABLE_OPTION(SupportsPMULL_128Bit, PMULL128, PMULL128);
  ENABLE_DISABLE_OPTION(SupportsRAND, RNG, RNG);
  ENABLE_DISABLE_OPTION(SupportsCLZERO, CLZERO, CLZERO);
  ENABLE_DISABLE_OPTION(SupportsAtomics, Atomics, ATOMICS);
  ENABLE_DISABLE_OPTION(SupportsFCMA, FCMA, FCMA);
  ENABLE_DISABLE_OPTION(SupportsFlagM, FlagM, FLAGM);
  ENABLE_DISABLE_OPTION(SupportsFlagM2, FlagM2, FLAGM2);
  ENABLE_DISABLE_OPTION(SupportsRPRES, RPRES, RPRES);
  ENABLE_DISABLE_OPTION(SupportsSVEBitPerm, SVEBITPERM, SVEBITPERM);
  ENABLE_DISABLE_OPTION(SupportsPreserveAllABI, PRESERVEALLABI, PRESERVEALLABI);
  GET_SINGLE_OPTION(Crypto, CRYPTO);

#undef ENABLE_DISABLE_OPTION
#undef GET_SINGLE_OPTION

  if (EnableCrypto) {
    Features->SupportsAES = true;
    Features->SupportsCRC = true;
    Features->SupportsSHA = true;
    Features->SupportsPMULL_128Bit = true;
    Features->SupportsAES256 = true;
  } else if (DisableCrypto) {
    Features->SupportsAES = false;
    Features->SupportsCRC = false;
    Features->SupportsSHA = false;
    Features->SupportsPMULL_128Bit = false;
    Features->SupportsAES256 = false;
  }

  ///< Only force enable SVE256 if SVE is already enabled and ForceSVEWidth is set to >= 256.
  Features->SupportsSVE256 = ForceSVEWidth && ForceSVEWidth >= 256;
}

FEXCore::HostFeatures FetchHostFeatures(vixl::CPUFeatures Features, bool SupportsCacheMaintenanceOps, uint64_t CTR, uint64_t MIDR) {
  FEXCore::HostFeatures HostFeatures;

  FEX_CONFIG_OPT(ForceSVEWidth, FORCESVEWIDTH);
  FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);

  HostFeatures.SupportsCacheMaintenanceOps = SupportsCacheMaintenanceOps;

  HostFeatures.SupportsAES = Features.Has(vixl::CPUFeatures::Feature::kAES);
  HostFeatures.SupportsCRC = Features.Has(vixl::CPUFeatures::Feature::kCRC32);
  HostFeatures.SupportsSHA = Features.Has(vixl::CPUFeatures::Feature::kSHA1) && Features.Has(vixl::CPUFeatures::Feature::kSHA2);
  HostFeatures.SupportsAtomics = Features.Has(vixl::CPUFeatures::Feature::kAtomics);
  HostFeatures.SupportsRAND = Features.Has(vixl::CPUFeatures::Feature::kRNG);

  // Only supported when FEAT_AFP is supported
  HostFeatures.SupportsAFP = Features.Has(vixl::CPUFeatures::Feature::kAFP);
  HostFeatures.SupportsRCPC = Features.Has(vixl::CPUFeatures::Feature::kRCpc);
  HostFeatures.SupportsTSOImm9 = Features.Has(vixl::CPUFeatures::Feature::kRCpcImm);
  HostFeatures.SupportsPMULL_128Bit = Features.Has(vixl::CPUFeatures::Feature::kPmull1Q);
  HostFeatures.SupportsCSSC = Features.Has(vixl::CPUFeatures::Feature::kCSSC);
  HostFeatures.SupportsFCMA = Features.Has(vixl::CPUFeatures::Feature::kFcma);
  HostFeatures.SupportsFlagM = Features.Has(vixl::CPUFeatures::Feature::kFlagM);
  HostFeatures.SupportsFlagM2 = Features.Has(vixl::CPUFeatures::Feature::kAXFlag);
  HostFeatures.SupportsRPRES = Features.Has(vixl::CPUFeatures::Feature::kRPRES);
  HostFeatures.SupportsSVEBitPerm = Features.Has(vixl::CPUFeatures::Feature::kSVEBitPerm);

#ifdef VIXL_SIMULATOR
  // Hardcode enable SVE with 256-bit wide registers.
  HostFeatures.SupportsSVE128 = ForceSVEWidth() ? ForceSVEWidth() >= 128 : true;
  HostFeatures.SupportsSVE256 = ForceSVEWidth() ? ForceSVEWidth() >= 256 : true;
#else
  HostFeatures.SupportsSVE128 = Features.Has(vixl::CPUFeatures::Feature::kSVE2);
  HostFeatures.SupportsSVE256 = Features.Has(vixl::CPUFeatures::Feature::kSVE2) && ReadSVEVectorLengthInBits() >= 256;
#endif
  HostFeatures.SupportsAVX = true;

  HostFeatures.SupportsAES256 = HostFeatures.SupportsAVX && HostFeatures.SupportsAES;

  if (!HostFeatures.SupportsAtomics) {
    WARN_ONCE_FMT("Host CPU doesn't support atomics. Expect bad performance");
  }

#ifdef _M_ARM_64
  // Test if this CPU supports float exception trapping by attempting to enable
  // On unsupported these bits are architecturally defined as RAZ/WI
  constexpr uint32_t ExceptionEnableTraps = (1U << 8) |  // Invalid Operation float exception trap enable
                                            (1U << 9) |  // Divide by zero float exception trap enable
                                            (1U << 10) | // Overflow float exception trap enable
                                            (1U << 11) | // Underflow float exception trap enable
                                            (1U << 12) | // Inexact float exception trap enable
                                            (1U << 15);  // Input Denormal float exception trap enable

  uint32_t OriginalFPCR = GetFPCR();
  uint32_t FPCR = OriginalFPCR | ExceptionEnableTraps;
  SetFPCR(FPCR);
  FPCR = GetFPCR();
  HostFeatures.SupportsFloatExceptions = (FPCR & ExceptionEnableTraps) == ExceptionEnableTraps;

  // Set FPCR back to original just in case anything changed
  SetFPCR(OriginalFPCR);

  if (HostFeatures.SupportsRAND) {
    constexpr uint32_t Implementer_QCOM = 0x51;
    constexpr uint32_t PartNum_Oryon1 = 0x001;
    const uint32_t MIDR_Implementer = (MIDR >> 24) & 0xFF;
    const uint32_t MIDR_PartNum = (MIDR >> 4) & 0xFFF;
    if (MIDR_Implementer == Implementer_QCOM && MIDR_PartNum == PartNum_Oryon1) {
      // Work around an errata in Qualcomm's Oryon.
      // While this CPU implements the RAND extension:
      // - The RNDR register works.
      // - The RNDRRS register will never read a random number. (Always return failure)
      // This is contrary to x86 RNG behaviour where it allows spurious failure with RDSEED, but guarantees eventual success.
      // This manifested itself on Linux when an x86 processor failed to guarantee forward progress and boot of services would infinite
      // loop. Just disable this extension if this CPU is detected.
      HostFeatures.SupportsRAND = false;
    }
  }
#endif

#ifdef VIXL_SIMULATOR
  // simulator doesn't support dc(ZVA)
  HostFeatures.SupportsCLZERO = false;
  // Simulator doesn't support SHA
  HostFeatures.SupportsSHA = false;
#else
  // Check if we can support cacheline clears
  uint32_t DCZID = GetDCZID();
  if ((DCZID & DCZID_DZP_MASK) == 0) {
    uint32_t DCZID_Log2 = DCZID & DCZID_BS_MASK;
    uint32_t DCZID_Bytes = (1 << DCZID_Log2) * sizeof(uint32_t);
    // If the DC ZVA size matches the emulated cache line size
    // This means we can use the instruction
    constexpr static uint64_t CACHELINE_SIZE = 64;
    HostFeatures.SupportsCLZERO = DCZID_Bytes == CACHELINE_SIZE;
  }
#endif

  if (CTR) {
    HostFeatures.DCacheLineSize = 4 << ((CTR >> 16) & 0xF);
    HostFeatures.ICacheLineSize = 4 << (CTR & 0xF);
  } else {
    HostFeatures.DCacheLineSize = HostFeatures.ICacheLineSize = 64;
  }

#if defined(_M_X86_64) && !defined(VIXL_SIMULATOR)
  Xbyak::util::Cpu X86Features {};
  HostFeatures.SupportsAES = X86Features.has(Xbyak::util::Cpu::tAESNI);
  HostFeatures.SupportsCRC = X86Features.has(Xbyak::util::Cpu::tSSE42);
  HostFeatures.SupportsRAND = X86Features.has(Xbyak::util::Cpu::tRDRAND) && X86Features.has(Xbyak::util::Cpu::tRDSEED);
  HostFeatures.SupportsRCPC = true;
  HostFeatures.SupportsTSOImm9 = true;
  HostFeatures.SupportsAVX = true;
  HostFeatures.SupportsSHA = X86Features.has(Xbyak::util::Cpu::tSHA);
  HostFeatures.SupportsPMULL_128Bit = X86Features.has(Xbyak::util::Cpu::tPCLMULQDQ);
  HostFeatures.SupportsAES256 = HostFeatures.SupportsAES && X86Features.has(Xbyak::util::Cpu::tVAES);

  // xbyak doesn't know how to check for CLZero
  // First ensure we support a new enough extended CPUID function range

  uint32_t data[4];
  Xbyak::util::Cpu::getCpuid(0x8000'0000, data);
  if (data[0] >= 0x8000'0008U) {
    // CLZero defined in 8000_00008_EBX[bit 0]
    Xbyak::util::Cpu::getCpuid(0x8000'0008, data);
    HostFeatures.SupportsCLZERO = data[1] & 1;
  }

  HostFeatures.SupportsAFP = true;
  HostFeatures.SupportsFloatExceptions = true;
#endif
  HostFeatures.SupportsPreserveAllABI = FEX_HAS_PRESERVE_ALL_ATTR;

  if (!Is64BitMode()) {
    ///< Always disable AVX and AVX2 in 32-bit mode.
    // When AVX256 is enabled, signal frames start using significantly more stack space.
    //   - 16bytes * 16 registers = 256 bytes for XMM registers.
    //   - 32bytes * 16 registers = 512 bytes for YMM registers.
    // There are known game failures on real x86 hardware where a 32-bit game is running up against the wall on stack space on non-AVX
    // hardware and then explodes when run on AVX hardware. This is to guard against that.
    HostFeatures.SupportsAVX = false;
  }

  OverrideFeatures(&HostFeatures, ForceSVEWidth());
  return HostFeatures;
}

FEXCore::HostFeatures FetchHostFeatures() {
#ifdef VIXL_SIMULATOR
  auto Features = vixl::CPUFeatures::All();
  // Vixl simulator doesn't support AFP.
  Features.Remove(vixl::CPUFeatures::Feature::kAFP);
  // Vixl simulator doesn't support RPRES.
  Features.Remove(vixl::CPUFeatures::Feature::kRPRES);
#else
  auto Features = vixl::CPUFeatures::InferFromOS();
#endif

  uint64_t CTR = 0;
  uint64_t MIDR = 0;
#ifdef _M_ARM_64
  // We need to get the CPU's cache line size
  // We expect sane targets that have correct cacheline sizes across clusters
  __asm volatile("mrs %[ctr], ctr_el0" : [ctr] "=r"(CTR));
  __asm volatile("mrs %[midr], midr_el1" : [midr] "=r"(MIDR));
#endif

  return FetchHostFeatures(Features, true, CTR, MIDR);
}
} // namespace FEX
