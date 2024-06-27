// SPDX-License-Identifier: MIT
#include "Interface/Core/CPUID.h"
#include <FEXCore/Core/HostFeatures.h>

#include "aarch64/assembler-aarch64.h"
#include "aarch64/cpu-aarch64.h"
#include "aarch64/disasm-aarch64.h"
#include "aarch64/assembler-aarch64.h"

#ifdef _M_X86_64
#define XBYAK64
#define XBYAK_NO_EXCEPTION
#include <FEXCore/fextl/list.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/unordered_set.h>

#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>
#endif

namespace FEXCore {

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

static uint32_t GetMIDR() {
  uint64_t Result {};
  __asm("mrs %[Res], MIDR_EL1" : [Res] "=r"(Result));
  return Result;
}

#else
static uint32_t GetDCZID() {
  // Return unsupported
  return DCZID_DZP_MASK;
}
#endif

static void OverrideFeatures(HostFeatures* Features) {
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
  ENABLE_DISABLE_OPTION(SupportsPreserveAllABI, PRESERVEALLABI, PRESERVEALLABI);
  GET_SINGLE_OPTION(Crypto, CRYPTO);
  FEX_CONFIG_OPT(ForceSVEWidth, FORCESVEWIDTH);

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
  Features->SupportsSVE256 = ForceSVEWidth() && ForceSVEWidth() >= 256;
}

HostFeatures::HostFeatures() {
#ifdef VIXL_SIMULATOR
  auto Features = vixl::CPUFeatures::All();
  // Vixl simulator doesn't support AFP.
  Features.Remove(vixl::CPUFeatures::Feature::kAFP);
  // Vixl simulator doesn't support RPRES.
  Features.Remove(vixl::CPUFeatures::Feature::kRPRES);
#elif !defined(_WIN32)
  auto Features = vixl::CPUFeatures::InferFromOS();
#else
  // Need to use ID registers in WINE.
  auto Features = vixl::CPUFeatures::InferFromIDRegisters();
#endif

  SupportsAES = Features.Has(vixl::CPUFeatures::Feature::kAES);
  SupportsCRC = Features.Has(vixl::CPUFeatures::Feature::kCRC32);
  SupportsSHA = Features.Has(vixl::CPUFeatures::Feature::kSHA1) && Features.Has(vixl::CPUFeatures::Feature::kSHA2);
  SupportsAtomics = Features.Has(vixl::CPUFeatures::Feature::kAtomics);
  SupportsRAND = Features.Has(vixl::CPUFeatures::Feature::kRNG);

  // Only supported when FEAT_AFP is supported
  SupportsAFP = Features.Has(vixl::CPUFeatures::Feature::kAFP);
  SupportsRCPC = Features.Has(vixl::CPUFeatures::Feature::kRCpc);
  SupportsTSOImm9 = Features.Has(vixl::CPUFeatures::Feature::kRCpcImm);
  SupportsPMULL_128Bit = Features.Has(vixl::CPUFeatures::Feature::kPmull1Q);
  SupportsCSSC = Features.Has(vixl::CPUFeatures::Feature::kCSSC);
  SupportsFCMA = Features.Has(vixl::CPUFeatures::Feature::kFcma);
  SupportsFlagM = Features.Has(vixl::CPUFeatures::Feature::kFlagM);
  SupportsFlagM2 = Features.Has(vixl::CPUFeatures::Feature::kAXFlag);
  SupportsRPRES = Features.Has(vixl::CPUFeatures::Feature::kRPRES);

  Supports3DNow = true;
  SupportsSSE4A = true;
#ifdef VIXL_SIMULATOR
  // Hardcode enable SVE with 256-bit wide registers.
  SupportsSVE128 = true;
  SupportsSVE256 = true;
#else
  SupportsSVE128 = Features.Has(vixl::CPUFeatures::Feature::kSVE2);
  SupportsSVE256 = Features.Has(vixl::CPUFeatures::Feature::kSVE2) && vixl::aarch64::CPU::ReadSVEVectorLengthInBits() >= 256;
#endif
  SupportsAVX = true;

  SupportsAES256 = SupportsAVX && SupportsAES;

  SupportsBMI1 = true;
  SupportsBMI2 = true;
  SupportsCLWB = true;

  if (!SupportsAtomics) {
    WARN_ONCE_FMT("Host CPU doesn't support atomics. Expect bad performance");
  }

#ifdef _M_ARM_64
  // We need to get the CPU's cache line size
  // We expect sane targets that have correct cacheline sizes across clusters
  uint64_t CTR;
  __asm volatile("mrs %[ctr], ctr_el0" : [ctr] "=r"(CTR));

  DCacheLineSize = 4 << ((CTR >> 16) & 0xF);
  ICacheLineSize = 4 << (CTR & 0xF);

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
  SupportsFloatExceptions = (FPCR & ExceptionEnableTraps) == ExceptionEnableTraps;

  // Set FPCR back to original just in case anything changed
  SetFPCR(OriginalFPCR);

  if (SupportsRAND) {
    const auto MIDR = GetMIDR();
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
      SupportsRAND = false;
    }
  }
#endif

#ifdef VIXL_SIMULATOR
  // simulator doesn't support dc(ZVA)
  SupportsCLZERO = false;
  // Simulator doesn't support SHA
  SupportsSHA = false;
#else
  // Check if we can support cacheline clears
  uint32_t DCZID = GetDCZID();
  if ((DCZID & DCZID_DZP_MASK) == 0) {
    uint32_t DCZID_Log2 = DCZID & DCZID_BS_MASK;
    uint32_t DCZID_Bytes = (1 << DCZID_Log2) * sizeof(uint32_t);
    // If the DC ZVA size matches the emulated cache line size
    // This means we can use the instruction
    SupportsCLZERO = DCZID_Bytes == CPUIDEmu::CACHELINE_SIZE;
  }
#endif

#if defined(_M_X86_64)
  // Hardcoded cacheline size.
  DCacheLineSize = 64U;
  ICacheLineSize = 64U;

#if !defined(VIXL_SIMULATOR)
  Xbyak::util::Cpu X86Features {};
  SupportsAES = X86Features.has(Xbyak::util::Cpu::tAESNI);
  SupportsCRC = X86Features.has(Xbyak::util::Cpu::tSSE42);
  SupportsRAND = X86Features.has(Xbyak::util::Cpu::tRDRAND) && X86Features.has(Xbyak::util::Cpu::tRDSEED);
  SupportsRCPC = true;
  SupportsTSOImm9 = true;
  Supports3DNow = X86Features.has(Xbyak::util::Cpu::t3DN) && X86Features.has(Xbyak::util::Cpu::tE3DN);
  SupportsSSE4A = X86Features.has(Xbyak::util::Cpu::tSSE4a);
  SupportsAVX = true;
  SupportsSHA = X86Features.has(Xbyak::util::Cpu::tSHA);
  SupportsBMI1 = X86Features.has(Xbyak::util::Cpu::tBMI1);
  SupportsBMI2 = X86Features.has(Xbyak::util::Cpu::tBMI2);
  SupportsCLWB = X86Features.has(Xbyak::util::Cpu::tCLWB);
  SupportsPMULL_128Bit = X86Features.has(Xbyak::util::Cpu::tPCLMULQDQ);
  SupportsAES256 = SupportsAES && X86Features.has(Xbyak::util::Cpu::tVAES);

  // xbyak doesn't know how to check for CLZero
  // First ensure we support a new enough extended CPUID function range

  uint32_t data[4];
  Xbyak::util::Cpu::getCpuid(0x8000'0000, data);
  if (data[0] >= 0x8000'0008U) {
    // CLZero defined in 8000_00008_EBX[bit 0]
    Xbyak::util::Cpu::getCpuid(0x8000'0008, data);
    SupportsCLZERO = data[1] & 1;
  }

  SupportsAFP = true;
  SupportsFloatExceptions = true;
#endif
#endif
  SupportsPreserveAllABI = FEXCORE_HAS_PRESERVE_ALL_ATTR;
  OverrideFeatures(this);
}
} // namespace FEXCore
