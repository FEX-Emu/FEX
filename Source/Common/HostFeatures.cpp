// SPDX-License-Identifier: MIT
#include "Common/CPUInfo.h"
#include "Common/HostFeatures.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/HostFeatures.h>
#include <FEXCore/Utils/FileLoading.h>
#include <FEXCore/Utils/StringUtils.h>

#include <range/v3/view/split.hpp>
#include <range/v3/view/transform.hpp>

#ifdef _M_X86_64
#include "Common/X86Features.h"
#endif

namespace FEX {

void FillMIDRInformationViaLinux(FEXCore::HostFeatures* Features) {
  auto Cores = FEX::CPUInfo::CalculateNumberOfCPUs();
  Features->CPUMIDRs.resize(Cores);
#ifdef _M_ARM_64
  for (size_t i = 0; i < Cores; ++i) {
    std::error_code ec {};
    fextl::string MIDRPath = fextl::fmt::format("/sys/devices/system/cpu/cpu{}/regs/identification/midr_el1", i);
    std::array<char, 18> Data;
    // Needs to be a fixed size since depending on kernel it will try to read a full page of data and fail
    // Only read 18 bytes for a 64bit value prefixed with 0x
    if (FEXCore::FileLoading::LoadFileToBuffer(MIDRPath, Data) == sizeof(Data)) {
      uint64_t MIDR {};
      auto Results = std::from_chars(Data.data() + 2, Data.data() + sizeof(Data), MIDR, 16);
      if (Results.ec == std::errc()) {
        // Truncate to 32-bits, top 32-bits are all reserved in MIDR
        Features->CPUMIDRs[i] = static_cast<uint32_t>(MIDR);
      }
    }
  }
#endif
}

#if defined(_M_ARM_64) && !defined(VIXL_SIMULATOR)
__attribute__((naked)) static uint64_t ReadSVEVectorLengthInBits() {
  ///< Can't use rdvl instruction directly because compilers will complain that sve/sme is required.
  __asm(R"(
  .word 0x04bf5100 // rdvl x0, #8
  ret;
  )");
}
#else
[[maybe_unused]]
static int ReadSVEVectorLengthInBits() {
  // Return unsupported
  return 0;
}
#endif

#ifdef _M_ARM_64
#define GetSysReg(name, reg)                         \
  static uint64_t Get_##name() {                     \
    uint64_t Result {};                              \
    __asm("mrs %[Res], " #reg : [Res] "=r"(Result)); \
    return Result;                                   \
  }

GetSysReg(ISAR0_EL1, ID_AA64ISAR0_EL1);
GetSysReg(PFR0_EL1, ID_AA64PFR0_EL1);
GetSysReg(PFR1_EL1, ID_AA64PFR1_EL1);
GetSysReg(MIDR_EL1, MIDR_EL1);
GetSysReg(ISAR1_EL1, ID_AA64ISAR1_EL1);
GetSysReg(MMFR0_EL1, ID_AA64MMFR0_EL1);
GetSysReg(MMFR2_EL1, ID_AA64MMFR2_EL1);
GetSysReg(ZFR0_EL1, s3_0_c0_c4_4); // Can't request by name
GetSysReg(MMFR1_EL1, ID_AA64MMFR1_EL1);
GetSysReg(ISAR2_EL1, ID_AA64ISAR2_EL1);
GetSysReg(DCZID_EL0, DCZID_EL0);

class CPUFeaturesFromID final : public FEX::CPUFeatures {
public:
  CPUFeaturesFromID() {
    ISAR0.SetReg(Get_ISAR0_EL1());
    PFR0.SetReg(Get_PFR0_EL1());
    PFR1.SetReg(Get_PFR1_EL1());
    MIDR.SetReg(Get_MIDR_EL1());
    ISAR1.SetReg(Get_ISAR1_EL1());
    MMFR0.SetReg(Get_MMFR0_EL1());
    MMFR2.SetReg(Get_MMFR2_EL1());
    MMFR1.SetReg(Get_MMFR1_EL1());
    ISAR2.SetReg(Get_ISAR2_EL1());
    DCZID.SetReg(Get_DCZID_EL0());

    if (PFR0.SupportsSVE()) {
      // Can only query if SVE is supported.
      ZFR0.SetReg(Get_ZFR0_EL1());
    }
    FillFeatureFlags();

    if (Supports(CPUFeatures::Feature::SVE2)) {
      SVEVL.SetReg(ReadSVEVectorLengthInBits());
    }
  }
};

FEX::CPUFeatures GetCPUFeaturesFromIDRegisters() {
  return CPUFeaturesFromID {};
}
#endif

class CPUFeaturesFromConfig final : public FEX::CPUFeatures {
public:
  CPUFeaturesFromConfig(std::string_view Config) {
    auto to_string_view = [](auto rng) {
      return std::string_view(&*rng.begin(), ranges::distance(rng));
    };

    for (auto Option : ranges::views::split(Config, ',') | ranges::views::transform(to_string_view)) {
      auto OptionData = ranges::views::split(Option, '=') | ranges::views::transform(to_string_view);
      auto OptionDataBegin = ranges::begin(OptionData);
      auto OptionDataEnd = ranges::end(OptionData);

      if (OptionDataBegin == OptionDataEnd) {
        continue;
      }

      auto Key = *OptionDataBegin;
      if (Key.empty()) {
        continue;
      }

      ++OptionDataBegin;
      if (OptionDataBegin == OptionDataEnd) {
        continue;
      }
      auto Value = *OptionDataBegin;
      uint64_t ValueHex {};
      char* str_end {};
      ValueHex = std::strtoull(Value.data(), &str_end, 16);

      if (str_end == Value.data()) {
        LogMan::Msg::EFmt("Couldn't parse '{}={}'\n", Key, Value);
        continue;
      }

      if (Key == "isar0") {
        ISAR0.SetReg(ValueHex);
      } else if (Key == "isar1") {
        ISAR1.SetReg(ValueHex);
      } else if (Key == "isar2") {
        ISAR2.SetReg(ValueHex);
      } else if (Key == "pfr0") {
        PFR0.SetReg(ValueHex);
      } else if (Key == "pfr1") {
        PFR1.SetReg(ValueHex);
      } else if (Key == "midr") {
        MIDR.SetReg(ValueHex);
      } else if (Key == "mmfr0") {
        MMFR0.SetReg(ValueHex);
      } else if (Key == "mmfr1") {
        MMFR1.SetReg(ValueHex);
      } else if (Key == "mmfr2") {
        MMFR2.SetReg(ValueHex);
      } else if (Key == "zfr0") {
        ZFR0.SetReg(ValueHex);
      } else if (Key == "dczid") {
        DCZID.SetReg(ValueHex);
      } else if (Key == "svevl") {
        SVEVL.SetReg(ValueHex);
      } else {
        LogMan::Msg::EFmt("Unknown Key: {}", Key);
      }
    }

    FillFeatureFlags();
  }
};

FEX::CPUFeatures GetCPUFeaturesFromConfig(std::string_view Config) {
  return CPUFeaturesFromConfig {Config};
}

class CPUFeaturesAll final : public FEX::CPUFeatures {
public:
  CPUFeaturesAll() {
    // Special case, just set all feature flags
    for (uint32_t i = 0; i < FEXCore::ToUnderlying(FEX::CPUFeatures::Feature::MAX); ++i) {
      SetFeature(FEX::CPUFeatures::Feature {i});
    }

    // Report unsupported for DCZVA
    DCZID.SetReg(0b1'0000);
  }
};

void FEX::CPUFeatures::FillFeatureFlags() {
  // ISAR0
  if (ISAR0.SupportsAES()) {
    SetFeature(Feature::AES);
  }
  if (ISAR0.SupportsPMULL()) {
    SetFeature(Feature::PMULL);
  }
  if (ISAR0.SupportsSHA1()) {
    SetFeature(Feature::SHA1);
  }
  if (ISAR0.SupportsSHA2()) {
    SetFeature(Feature::SHA2);
  }
  if (ISAR0.SupportsSHA512()) {
    SetFeature(Feature::SHA512);
  }
  if (ISAR0.SupportsCRC32()) {
    SetFeature(Feature::CRC32);
  }
  if (ISAR0.SupportsLSE()) {
    SetFeature(Feature::LSE);
  }
  if (ISAR0.SupportsLSE128()) {
    SetFeature(Feature::LSE128);
  }
  if (ISAR0.SupportsTME()) {
    SetFeature(Feature::TME);
  }
  if (ISAR0.SupportsRDM()) {
    SetFeature(Feature::RDM);
  }
  if (ISAR0.SupportsSHA3()) {
    SetFeature(Feature::SHA3);
  }
  if (ISAR0.SupportsSM3()) {
    SetFeature(Feature::SM3);
  }
  if (ISAR0.SupportsSM4()) {
    SetFeature(Feature::SM4);
  }
  if (ISAR0.SupportsDotProd()) {
    SetFeature(Feature::DotProd);
  }
  if (ISAR0.SupportsFlagM()) {
    SetFeature(Feature::FlagM);
  }
  if (ISAR0.SupportsFlagM2()) {
    SetFeature(Feature::FlagM2);
  }
  if (ISAR0.SupportsRNDR()) {
    SetFeature(Feature::RNDR);
  }

  // PFR0
  if (PFR0.SupportsFP()) {
    SetFeature(Feature::FP);
  }
  if (PFR0.SupportsHP()) {
    SetFeature(Feature::FP16);
  }
  if (PFR0.SupportsAdvSIMD()) {
    SetFeature(Feature::ASIMD);
  }
  if (PFR0.SupportsASIMDHP()) {
    SetFeature(Feature::ASIMD16);
  }
  if (PFR0.SupportsRAS()) {
    SetFeature(Feature::RAS);
  }
  if (PFR0.SupportsSVE()) {
    SetFeature(Feature::SVE);
  }
  if (PFR0.SupportsDIT()) {
    SetFeature(Feature::DIT);
  }
  if (PFR0.SupportsCSV2()) {
    SetFeature(Feature::CSV2);
  }
  if (PFR0.SupportsCSV3()) {
    SetFeature(Feature::CSV3);
  }

  // PFR1
  if (PFR1.SupportsBTI()) {
    SetFeature(Feature::BTI);
  }
  if (PFR1.SupportsSSBS()) {
    SetFeature(Feature::SSBS);
  }
  if (PFR1.SupportsSSBS()) {
    SetFeature(Feature::SSBS2);
  }
  if (PFR1.SupportsMTE()) {
    SetFeature(Feature::MTE);
  }
  if (PFR1.SupportsMTE2()) {
    SetFeature(Feature::MTE2);
  }
  if (PFR1.SupportsMTE3()) {
    SetFeature(Feature::MTE3);
  }
  if (PFR1.SupportsSME()) {
    SetFeature(Feature::SME);
  }
  if (PFR1.SupportsSME2()) {
    SetFeature(Feature::SME2);
  }

  // ISAR1
  if (ISAR1.SupportsDPB()) {
    SetFeature(Feature::DPB);
  }
  if (ISAR1.SupportsDPB2()) {
    SetFeature(Feature::DPB2);
  }
  if (ISAR1.SupportsJSCVT()) {
    SetFeature(Feature::JSCVT);
  }
  if (ISAR1.SupportsFCMA()) {
    SetFeature(Feature::FCMA);
  }
  if (ISAR1.SupportsLRCPC()) {
    SetFeature(Feature::LRCPC);
  }
  if (ISAR1.SupportsLRCPC2()) {
    SetFeature(Feature::LRCPC2);
  }
  if (ISAR1.SupportsLRCPC3()) {
    SetFeature(Feature::LRCPC3);
  }
  if (ISAR1.SupportsFRINTTS()) {
    SetFeature(Feature::FRINTTS);
  }
  if (ISAR1.SupportsSB()) {
    SetFeature(Feature::SB);
  }
  if (ISAR1.SupportsSPECRES()) {
    SetFeature(Feature::SPECRES);
  }
  if (ISAR1.SupportsSPECRES2()) {
    SetFeature(Feature::SPECRES2);
  }
  if (ISAR1.SupportsBF16()) {
    SetFeature(Feature::BF16);
  }
  if (ISAR1.SupportsSME_F64F64()) {
    SetFeature(Feature::SME_F64F64);
  }
  if (ISAR1.SupportsI8MM()) {
    SetFeature(Feature::I8MM);
  }
  if (ISAR1.SupportsXS()) {
    SetFeature(Feature::XS);
  }
  if (ISAR1.SupportsLS64()) {
    SetFeature(Feature::LS64);
  }
  if (ISAR1.SupportsLS64_V()) {
    SetFeature(Feature::LS64_V);
  }
  if (ISAR1.SupportsLS64_ACCDATA()) {
    SetFeature(Feature::LS64_ACCDATA);
  }

  // MMFR0
  if (MMFR0.SupportsECV()) {
    SetFeature(Feature::ECV);
  }

  // MMFR2
  if (MMFR2.SupportsLSE2()) {
    SetFeature(Feature::LSE2);
  }

  // ZFR0
  if (Supports(Feature::SVE)) {
    if (ZFR0.SupportsSVE2()) {
      SetFeature(Feature::SVE2);
    }
    if (ZFR0.SupportsSVE2_1()) {
      SetFeature(Feature::SVE2_1);
    }
    if (ZFR0.SupportsSVE_AES()) {
      SetFeature(Feature::SVE_AES);
    }
    if (ZFR0.SupportsSVE_PMULL128()) {
      SetFeature(Feature::SVE_PMULL128);
    }
    if (ZFR0.SupportsSVE_BitPerm()) {
      SetFeature(Feature::SVE_BitPerm);
    }
    if (ZFR0.SupportsSVE_BF16()) {
      SetFeature(Feature::SVE_BF16);
    }
    if (ZFR0.SupportsSVE_B16B16()) {
      SetFeature(Feature::SVE_B16B16);
    }
    if (ZFR0.SupportsSVE_SHA3()) {
      SetFeature(Feature::SVE_SHA3);
    }
    if (ZFR0.SupportsSVE_SM4()) {
      SetFeature(Feature::SVE_SM4);
    }
    if (ZFR0.SupportsSVE_I8MM()) {
      SetFeature(Feature::SVE_I8MM);
    }
    if (ZFR0.SupportsSVE_F32MM()) {
      SetFeature(Feature::SVE_F32MM);
    }
    if (ZFR0.SupportsSVE_F64MM()) {
      SetFeature(Feature::SVE_F64MM);
    }
  }

  // MMFR1
  if (MMFR1.SupportsAFP()) {
    SetFeature(Feature::AFP);
  }

  // ISAR2
  if (ISAR2.SupportsWFxt()) {
    SetFeature(Feature::WFxt);
  }
  if (ISAR2.SupportsRPRES()) {
    SetFeature(Feature::RPRES);
  }
  if (ISAR2.SupportsPACQARMA3()) {
    SetFeature(Feature::PACQARMA3);
  }
  if (ISAR2.SupportsMOPS()) {
    SetFeature(Feature::MOPS);
  }
  if (ISAR2.SupportsHBC()) {
    SetFeature(Feature::HBC);
  }
  if (ISAR2.SupportsCLRBHB()) {
    SetFeature(Feature::CLRBHB);
  }
  if (ISAR2.SupportsSYSREG128()) {
    SetFeature(Feature::SYSREG128);
  }
  if (ISAR2.SupportsSYSINSTR128()) {
    SetFeature(Feature::SYSINSTR128);
  }
  if (ISAR2.SupportsPRFMSLC()) {
    SetFeature(Feature::PRFMSLC);
  }
  if (ISAR2.SupportsRPRFM()) {
    SetFeature(Feature::RPRFM);
  }
  if (ISAR2.SupportsCSSC()) {
    SetFeature(Feature::CSSC);
  }
}

#ifdef _M_ARM_64
static uint32_t GetFPCR() {
  uint64_t Result {};
  __asm("mrs %[Res], FPCR" : [Res] "=r"(Result));
  return Result;
}

static void SetFPCR(uint64_t Value) {
  __asm("msr FPCR, %[Value]" ::[Value] "r"(Value));
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
  ENABLE_DISABLE_OPTION(SupportsFRINTTS, FRINTTS, FRINTTS);
  ENABLE_DISABLE_OPTION(SupportsRPRES, RPRES, RPRES);
  ENABLE_DISABLE_OPTION(SupportsSVEBitPerm, SVEBITPERM, SVEBITPERM);
  ENABLE_DISABLE_OPTION(SupportsPreserveAllABI, PRESERVEALLABI, PRESERVEALLABI);
  ENABLE_DISABLE_OPTION(SupportsWFXT, WFXT, WFXT);
  ENABLE_DISABLE_OPTION(Supports3DNow, 3DNOW, 3DNOW);
  ENABLE_DISABLE_OPTION(SupportsSSE4a, SSE4A, SSE4A);
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

static void HandleErrata(FEXCore::HostFeatures* HostFeatures, uint64_t MIDR) {
  constexpr uint32_t Implementer_ARM = 0x41;
  constexpr uint32_t PartNum_V2 = 0xd4f;
  constexpr uint32_t PartNum_V3 = 0xd84;
  constexpr uint32_t PartNum_V3AE = 0xd83;
  constexpr uint32_t PartNum_X3 = 0xd4e;
  constexpr uint32_t PartNum_X4 = 0xd82;
  constexpr uint32_t PartNum_X925 = 0xd85;

  constexpr uint32_t Implementer_QCOM = 0x51;
  constexpr uint32_t PartNum_Oryon1 = 0x001;

  auto GetMIDRImplementer = [](uint32_t MIDR) -> uint32_t {
    return (MIDR >> 24) & 0xFF;
  };

  auto GetMIDRPartNum = [](uint32_t MIDR) -> uint32_t {
    return (MIDR >> 4) & 0xFFF;
  };

  const uint32_t MIDR_Implementer = GetMIDRImplementer(MIDR);
  const uint32_t MIDR_PartNum = GetMIDRPartNum(MIDR);

#ifdef _M_ARM_64
  if (MIDR_Implementer == Implementer_QCOM && MIDR_PartNum == PartNum_Oryon1) {
    // Work around an errata in Qualcomm's Oryon.
    // While this CPU implements the RAND extension:
    // - The RNDR register works.
    // - The RNDRRS register will never read a random number. (Always return failure)
    // This is contrary to x86 RNG behaviour where it allows spurious failure with RDSEED, but guarantees eventual success.
    // This manifested itself on Linux when an x86 processor failed to guarantee forward progress and boot of services would infinite
    // loop. Just disable this extension if this CPU is detected.
    HostFeatures->SupportsRAND = false;
  }
#endif

  // The LDAPUR instruction suffers from significant performance issues on many ARM implementations. This is
  // listed in the official Cortex errata list as follows:
  //
  // 3877900
  // LDAPUR, LDAPURB, LDAPURH instructions have stricter memory ordering than required
  //
  // LDAPUR instructions execute with full Load-Acquire ordering instead of the relaxed ordering described
  // in the LDAPUR pseudocode. This might cause significant performance degradation in workloads that do
  // not require this stricter memory ordering. Note that this erratum only affects the unscaled versions of
  // LDAPUR (LDAPUR, LDAPURB, LDAPURH), and not LDAPR (LDAPR, LDAPRB, LDAPRH).
  //
  // The list of cores to disable its use on was taken from the following LLVM PR that accomplishes the same
  // thing: https://github.com/llvm/llvm-project/pull/124274
  for (uint32_t CoreIndex = 0; CoreIndex < HostFeatures->CPUMIDRs.size(); CoreIndex++) {
    const uint32_t CoreMIDR = HostFeatures->CPUMIDRs[CoreIndex];
    const uint32_t Core_MIDR_Implementer = GetMIDRImplementer(CoreMIDR);
    const uint32_t Core_MIDR_PartNum = GetMIDRPartNum(CoreMIDR);

    bool IgnoreLRCPC2 = (Core_MIDR_Implementer == Implementer_ARM) &&
                        ((Core_MIDR_PartNum == PartNum_V2) || (Core_MIDR_PartNum == PartNum_V3) || (Core_MIDR_PartNum == PartNum_X3) ||
                         (Core_MIDR_PartNum == PartNum_X4) || (Core_MIDR_PartNum == PartNum_X925) || (Core_MIDR_PartNum == PartNum_V3AE));

    if (IgnoreLRCPC2) {
      HostFeatures->SupportsTSOImm9 = false;
      break;
    }
  }
}

void FetchHostFeatures(FEX::CPUFeatures& Features, FEXCore::HostFeatures& HostFeatures, bool SupportsCacheMaintenanceOps, uint64_t CTR,
                       uint64_t MIDR) {
  FEX_CONFIG_OPT(ForceSVEWidth, FORCESVEWIDTH);
  FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);

  HostFeatures.SupportsCacheMaintenanceOps = SupportsCacheMaintenanceOps;

  HostFeatures.SupportsAES = Features.Supports(CPUFeatures::Feature::AES);
  HostFeatures.SupportsCRC = Features.Supports(CPUFeatures::Feature::CRC32);
  HostFeatures.SupportsSHA = Features.Supports(CPUFeatures::Feature::SHA1) && Features.Supports(CPUFeatures::Feature::SHA2);
  HostFeatures.SupportsAtomics = Features.Supports(CPUFeatures::Feature::LSE);
  HostFeatures.SupportsRAND = Features.Supports(CPUFeatures::Feature::RNDR);

  // Only supported when FEAT_AFP is supported
  HostFeatures.SupportsAFP = Features.Supports(CPUFeatures::Feature::AFP);
  HostFeatures.SupportsRCPC = Features.Supports(CPUFeatures::Feature::LRCPC);
  HostFeatures.SupportsTSOImm9 = Features.Supports(CPUFeatures::Feature::LRCPC2);
  HostFeatures.SupportsPMULL_128Bit = Features.Supports(CPUFeatures::Feature::PMULL);
  HostFeatures.SupportsCSSC = Features.Supports(CPUFeatures::Feature::CSSC);
  HostFeatures.SupportsFCMA = Features.Supports(CPUFeatures::Feature::FCMA);
  HostFeatures.SupportsFlagM = Features.Supports(CPUFeatures::Feature::FlagM);
  HostFeatures.SupportsFlagM2 = Features.Supports(CPUFeatures::Feature::FlagM2);
  HostFeatures.SupportsFRINTTS = Features.Supports(CPUFeatures::Feature::FRINTTS);
  HostFeatures.SupportsRPRES = Features.Supports(CPUFeatures::Feature::RPRES);
  HostFeatures.SupportsSVEBitPerm = Features.Supports(CPUFeatures::Feature::SVE_BitPerm);
  HostFeatures.SupportsECV = Features.Supports(CPUFeatures::Feature::ECV);
  HostFeatures.SupportsWFXT = Features.Supports(CPUFeatures::Feature::WFxt);

#ifdef VIXL_SIMULATOR
  // Hardcode enable SVE with 256-bit wide registers.
  HostFeatures.SupportsSVE128 = ForceSVEWidth() ? ForceSVEWidth() >= 128 : true;
  HostFeatures.SupportsSVE256 = ForceSVEWidth() ? ForceSVEWidth() >= 256 : true;
#else
  HostFeatures.SupportsSVE128 = Features.Supports(CPUFeatures::Feature::SVE2);
  HostFeatures.SupportsSVE256 = Features.Supports(CPUFeatures::Feature::SVE2) && Features.GetSVEVectorLengthInBits() >= 256;
#endif
  HostFeatures.SupportsAVX = true;

#ifdef _WIN32
  // Disable 3DNow! by default to better match the set of extensions exposed on modern CPUs.
  // This works around a bug that manifests in some games using native d3dx9 DLLs (most easily reproduced in WoW64 builds).
  // For example, Fallout: New Vegas and some old EA games will run with a blackscreen.
  HostFeatures.Supports3DNow = false;
#else
  HostFeatures.Supports3DNow = true;
#endif

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
#endif

#ifdef VIXL_SIMULATOR
  // simulator has a hardcoded ZVA size of 64-bytes.
  HostFeatures.SupportsCLZERO = true;
  HostFeatures.SupportsAES = true;
  HostFeatures.SupportsCRC = true;
  HostFeatures.SupportsAVX = true;
  HostFeatures.SupportsSHA = true;
  HostFeatures.SupportsPMULL_128Bit = true;
  HostFeatures.SupportsAES256 = true;
#else
  // Check if we can support cacheline clears
  if (Features.GetDCZID().SupportsDCZVA()) {
    // If the DC ZVA size matches the emulated cache line size
    // This means we can use the instruction
    constexpr static uint64_t CACHELINE_SIZE = 64;
    HostFeatures.SupportsCLZERO = Features.GetDCZID().BlockSizeInBytes() == CACHELINE_SIZE;
  }
#endif

  if (CTR) {
    HostFeatures.DCacheLineSize = 4 << ((CTR >> 16) & 0xF);
    HostFeatures.ICacheLineSize = 4 << (CTR & 0xF);
  } else {
    HostFeatures.DCacheLineSize = HostFeatures.ICacheLineSize = 64;
  }

#if defined(_M_X86_64) && !defined(VIXL_SIMULATOR)
  FEX::X86::Features Feature {};
  HostFeatures.SupportsAES = Feature.Feat_aes;
  HostFeatures.SupportsCRC = Feature.Feat_crc;
  HostFeatures.SupportsRAND = Feature.Feat_rand;
  HostFeatures.SupportsRCPC = true;
  HostFeatures.SupportsTSOImm9 = true;
  HostFeatures.SupportsAVX = Feature.Feat_avx;
  HostFeatures.SupportsSHA = Feature.Feat_sha;
  HostFeatures.SupportsPMULL_128Bit = Feature.Feat_pclmulqdq;
  HostFeatures.SupportsAES256 = Feature.Feat_aes;
  HostFeatures.SupportsCLZERO = Feature.Feat_clzero;

  HostFeatures.SupportsAFP = true;
  HostFeatures.SupportsFloatExceptions = true;
#endif
  HostFeatures.SupportsPreserveAllABI = FEX_HAS_PRESERVE_ALL_ATTR;

  HandleErrata(&HostFeatures, MIDR);
  OverrideFeatures(&HostFeatures, ForceSVEWidth());
}

FEXCore::HostFeatures FetchHostFeatures() {
  FEX_CONFIG_OPT(CPUFeatureRegisters, CPUFEATUREREGISTERS);

  CPUFeatures Features {};
  if (!CPUFeatureRegisters().empty()) {
    Features = GetCPUFeaturesFromConfig(CPUFeatureRegisters());
  } else {
#ifdef _M_X86_64
    Features = CPUFeaturesAll {};

    // Vixl simulator doesn't support AFP.
    Features.RemoveFeature(CPUFeatures::Feature::AFP);
    // Vixl simulator doesn't support RPRES.
    Features.RemoveFeature(CPUFeatures::Feature::RPRES);
#else
    Features = GetCPUFeaturesFromIDRegisters();
#endif
  }

  uint64_t CTR = 0;
  uint64_t MIDR = 0;
#ifdef _M_ARM_64
  // We need to get the CPU's cache line size
  // We expect sane targets that have correct cacheline sizes across clusters
  __asm volatile("mrs %[ctr], ctr_el0" : [ctr] "=r"(CTR));
  __asm volatile("mrs %[midr], midr_el1" : [midr] "=r"(MIDR));
#endif

  FEXCore::HostFeatures HostFeatures = {};
  FillMIDRInformationViaLinux(&HostFeatures);
  FetchHostFeatures(Features, HostFeatures, true, CTR, MIDR);

  HostFeatures.SupportsCPUIndexInTPIDRRO = false;
  return HostFeatures;
}
} // namespace FEX
