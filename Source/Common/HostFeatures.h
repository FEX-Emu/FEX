// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Core/HostFeatures.h>
#include <FEXCore/Utils/EnumUtils.h>

#include <cstddef>

namespace FEX {
class CPUFeatures {
public:
  class FeatureReg {
  public:
    void SetReg(uint64_t _Reg) {
      Reg = _Reg;
    }

    uint64_t Get() const {
      return Reg;
    }
  protected:
    // All feature flag fields are 4-bits.
    uint64_t GetField(uint64_t Offset) const {
      return (Reg >> Offset) & 0b1111;
    }
    uint64_t Reg {};
  };

  enum class Feature : uint32_t {
    // ISAR0
    AES,
    PMULL,
    SHA1,
    SHA2,
    SHA512,
    CRC32,
    LSE,
    LSE128,
    TME,
    RDM,
    SHA3,
    SM3,
    SM4,
    DotProd,
    FlagM,
    FlagM2,
    RNDR,
    // PFR0
    FP,
    FP16,
    ASIMD,
    ASIMD16,
    RAS,
    SVE,
    DIT,
    CSV2,
    CSV3,
    // PFR1
    BTI,
    SSBS,
    SSBS2,
    MTE,
    MTE2,
    MTE3,
    SME,
    SME2,
    // ISAR1
    DPB,
    DPB2,
    JSCVT,
    FCMA,
    LRCPC,
    LRCPC2,
    LRCPC3,
    FRINTTS,
    SB,
    SPECRES,
    SPECRES2,
    BF16,
    SME_F64F64,
    I8MM,
    XS,
    LS64,
    LS64_V,
    LS64_ACCDATA,
    // MMFR0
    ECV,
    // MMFR2
    LSE2,
    // ZFR0
    SVE2,
    SVE2_1,
    SVE_AES,
    SVE_PMULL128,
    SVE_BitPerm,
    SVE_BF16,
    SVE_B16B16,
    SVE_SHA3,
    SVE_SM4,
    SVE_I8MM,
    SVE_F32MM,
    SVE_F64MM,
    // MMFR1
    AFP,
    // ISAR2
    WFxt,
    RPRES,
    PACQARMA3,
    MOPS,
    HBC,
    CLRBHB,
    SYSREG128,
    SYSINSTR128,
    PRFMSLC,
    RPRFM,
    CSSC,
    // Max indicator
    MAX,
  };

  class DCZIDReg final : public FeatureReg {
  public:
    bool SupportsDCZVA() const {
      return (Reg & DCZID_DZP_MASK) == 0;
    }

    uint32_t BlockSizeInBytes() const {
      uint32_t DCZID_Log2 = Reg & DCZID_BS_MASK;
      return (1 << DCZID_Log2) * sizeof(uint32_t);
    }

  private:
    // Data Zero Prohibited flag
    // 0b0 = ZVA/GVA/GZVA permitted
    // 0b1 = ZVA/GVA/GZVA prohibited
    [[maybe_unused]] constexpr static uint32_t DCZID_DZP_MASK = 0b1'0000;
    // Log2 of the blocksize in 32-bit words
    [[maybe_unused]] constexpr static uint32_t DCZID_BS_MASK = 0b0'1111;
  };

  // This list is informed by Linux kernel's `Documentation/arch/arm64/cpu-feature-registers.rst`
  enum class FeatureRegType {
    ISAR0_EL1,
    PFR0_EL1,
    PFR1_EL1,
    MIDR_EL1,
    ISAR1_EL1,
    MMFR0_EL1,
    MMFR2_EL1,
    ZFR0_EL1,
    MMFR1_EL1,
    ISAR2_EL1,
  };

#define FIELD_FETCHER(feature, field, minimum_field) \
  bool Supports##feature() const {                   \
    return GetField(field) >= minimum_field;         \
  }

  class ISAR0Reg final : public FeatureReg {
  public:
    FIELD_FETCHER(AES, AES, 0b0001);
    FIELD_FETCHER(PMULL, AES, 0b0010);

    FIELD_FETCHER(SHA1, SHA1, 0b0001);

    FIELD_FETCHER(SHA2, SHA2, 0b0001);
    FIELD_FETCHER(SHA512, SHA2, 0b0010);

    FIELD_FETCHER(CRC32, CRC32, 0b0001);

    FIELD_FETCHER(LSE, Atomic, 0b0010);
    FIELD_FETCHER(LSE128, Atomic, 0b0011);

    FIELD_FETCHER(TME, TME, 0b0001);

    FIELD_FETCHER(RDM, RDM, 0b0001);

    FIELD_FETCHER(SHA3, SHA3, 0b0001);

    FIELD_FETCHER(SM3, SM3, 0b0001);

    FIELD_FETCHER(SM4, SM4, 0b0001);

    FIELD_FETCHER(DotProd, DP, 0b0001);

    FIELD_FETCHER(FHM, FHM, 0b0001);

    FIELD_FETCHER(FlagM, TS, 0b0001);
    FIELD_FETCHER(FlagM2, TS, 0b0010);

    FIELD_FETCHER(TLBIOS, TLB, 0b0001);
    FIELD_FETCHER(TLBIRANGE, TLB, 0b0010);

    FIELD_FETCHER(RNDR, RNDR, 0b0001);

  private:
    enum Field {
      RES0 = 0 * 4,
      AES = 1 * 4,
      SHA1 = 2 * 4,
      SHA2 = 3 * 4,
      CRC32 = 4 * 4,
      Atomic = 5 * 4,
      TME = 6 * 4,
      RDM = 7 * 4,
      SHA3 = 8 * 4,
      SM3 = 9 * 4,
      SM4 = 10 * 4,
      DP = 11 * 4,
      FHM = 12 * 4,
      TS = 13 * 4,
      TLB = 14 * 4,
      RNDR = 15 * 4,
    };
  };

  class PFR0Reg final : public FeatureReg {
  public:
    FIELD_FETCHER(AA64_EL0, EL0, 0b0001);
    FIELD_FETCHER(AA32_EL0, EL0, 0b0010);

    FIELD_FETCHER(AA64_EL1, EL1, 0b0001);
    FIELD_FETCHER(AA32_EL1, EL1, 0b0010);

    FIELD_FETCHER(AA64_EL2, EL2, 0b0001);
    FIELD_FETCHER(AA32_EL2, EL2, 0b0010);

    FIELD_FETCHER(AA64_EL3, EL3, 0b0001);
    FIELD_FETCHER(AA32_EL3, EL3, 0b0010);

    bool SupportsFP() const {
      return GetField(FP) != 0b1111;
    }
    FIELD_FETCHER(HP, FP, 0b0001);

    bool SupportsAdvSIMD() const {
      return GetField(AdvSIMD) != 0b1111;
    }
    FIELD_FETCHER(ASIMDHP, AdvSIMD, 0b0001);

    FIELD_FETCHER(GIC4_0, GIC, 0b0001);
    FIELD_FETCHER(GIC4_1, GIC, 0b0011);

    FIELD_FETCHER(RAS, RAS, 0b0001);
    FIELD_FETCHER(RAS1_1, RAS, 0b0010);
    FIELD_FETCHER(RAS2, RAS, 0b0011);

    FIELD_FETCHER(SVE, SVE, 0b0001);

    FIELD_FETCHER(SEL2, SEL2, 0b0001);

    uint64_t MPAM_Major() const {
      return GetField(MPAM);
    }

    FIELD_FETCHER(AMU1, AMU, 0b0001);
    FIELD_FETCHER(AMU1_1, AMU, 0b0010);

    FIELD_FETCHER(DIT, DIT, 0b0001);

    FIELD_FETCHER(RME, RME, 0b0001);

    FIELD_FETCHER(CSV2, CSV2, 0b0001);
    FIELD_FETCHER(CSV2_2, CSV2, 0b0010);
    FIELD_FETCHER(CSV2_3, CSV2, 0b0011);

    FIELD_FETCHER(CSV3, CSV3, 0b0001);

  private:
    enum Field {
      EL0 = 0 * 4,
      EL1 = 1 * 4,
      EL2 = 2 * 4,
      EL3 = 3 * 4,
      FP = 4 * 4,
      AdvSIMD = 5 * 4,
      GIC = 6 * 4,
      RAS = 7 * 4,
      SVE = 8 * 4,
      SEL2 = 9 * 4,
      MPAM = 10 * 4,
      AMU = 11 * 4,
      DIT = 12 * 4,
      RME = 13 * 4,
      CSV2 = 14 * 4,
      CSV3 = 15 * 4,
    };
  };

  class PFR1Reg final : public FeatureReg {
  public:
    FIELD_FETCHER(BTI, BT, 0b0001);

    FIELD_FETCHER(SSBS, SSBS, 0b0001);
    FIELD_FETCHER(SSBS2, SSBS, 0b0010);

    FIELD_FETCHER(MTE, MTE, 0b0001);
    FIELD_FETCHER(MTE2, MTE, 0b0010);
    FIELD_FETCHER(MTE3, MTE, 0b0011);

    uint64_t RAS_Minor() const {
      return GetField(RAS_frac);
    }
    uint64_t MPAM_Minor() const {
      return GetField(MPAM_frac);
    }

    FIELD_FETCHER(SME, SME, 0b0001);
    FIELD_FETCHER(SME2, SME, 0b0010);

    FIELD_FETCHER(RNDR_trap, RNDR_trap, 0b0001);

    uint64_t CSV2_Minor() const {
      return GetField(CSV2_frac);
    }

    FIELD_FETCHER(NMI, NMI, 0b0001);

    uint64_t MTE_Minor() const {
      return GetField(MTE_frac);
    }

    FIELD_FETCHER(GCS, GCS, 0b0001);

    FIELD_FETCHER(THE, THE, 0b0001);

    FIELD_FETCHER(MTEX, MTEX, 0b0001);

    FIELD_FETCHER(DoubleFault2, DF2, 0b0001);

    FIELD_FETCHER(PFAR, PFAR, 0b0001);

  private:
    enum Field {
      BT = 0 * 4,
      SSBS = 1 * 4,
      MTE = 2 * 4,
      RAS_frac = 3 * 4,
      MPAM_frac = 4 * 4,
      RES0 = 5 * 4,
      SME = 6 * 4,
      RNDR_trap = 7 * 4,
      CSV2_frac = 8 * 4,
      NMI = 9 * 4,
      MTE_frac = 10 * 4,
      GCS = 11 * 4,
      THE = 12 * 4,
      MTEX = 13 * 4,
      DF2 = 14 * 4,
      PFAR = 15 * 4,
    };
  };

  class MIDRReg final : public FeatureReg {
  public:
    uint64_t GetRevision() const {
      return GetField(Revision);
    }
    uint64_t GetPartNum() const {
      return (Reg >> 4) & 0xFFF;
    }
    uint64_t GetArchitecture() const {
      return GetField(Architecture);
    }
    uint64_t GetVariant() const {
      return GetField(Variant);
    }
    uint64_t GetImplementer() const {
      return (Reg >> 24) & 0xFFFF;
    }

  private:
    enum Field {
      Revision = 0 * 4,
      // Partnum is 3 fields [15:4]
      Architecture = 4 * 4,
      Variant = 5 * 4,
      // Implementer is 2 fields [31:24]
      // Upper 32-bits is entirely reserved
    };
  };

  class ISAR1Reg final : public FeatureReg {
  public:
    FIELD_FETCHER(DPB, DPB, 0b0001);
    FIELD_FETCHER(DPB2, DPB, 0b0010);

    // Ignoring APA and API

    FIELD_FETCHER(JSCVT, JSCVT, 0b0001);

    FIELD_FETCHER(FCMA, FCMA, 0b0001);

    FIELD_FETCHER(LRCPC, LRCPC, 0b0001);
    FIELD_FETCHER(LRCPC2, LRCPC, 0b0010);
    FIELD_FETCHER(LRCPC3, LRCPC, 0b0011);

    // Ignoring GPA and GPI

    FIELD_FETCHER(FRINTTS, FRINTTS, 0b0001);

    FIELD_FETCHER(SB, SB, 0b0001);

    FIELD_FETCHER(SPECRES, SPECRES, 0b0001);
    FIELD_FETCHER(SPECRES2, SPECRES, 0b0010);

    FIELD_FETCHER(BF16, BF16, 0b0001);
    FIELD_FETCHER(SME_F64F64, BF16, 0b0010);

    FIELD_FETCHER(DGH, DGH, 0b0001);

    FIELD_FETCHER(I8MM, I8MM, 0b0001);

    FIELD_FETCHER(XS, XS, 0b0001);

    FIELD_FETCHER(LS64, LS64, 0b0001);
    FIELD_FETCHER(LS64_V, LS64, 0b0010);
    FIELD_FETCHER(LS64_ACCDATA, LS64, 0b0011);

  private:
    enum Field {
      DPB = 0 * 4,
      APA = 1 * 4,
      API = 2 * 4,
      JSCVT = 3 * 4,
      FCMA = 4 * 4,
      LRCPC = 5 * 4,
      GPA = 6 * 4,
      GPI = 7 * 4,
      FRINTTS = 8 * 4,
      SB = 9 * 4,
      SPECRES = 10 * 4,
      BF16 = 11 * 4,
      DGH = 12 * 4,
      I8MM = 13 * 4,
      XS = 14 * 4,
      LS64 = 15 * 4,
    };
  };

  class MMFR0Reg final : public FeatureReg {
  public:
    FIELD_FETCHER(ECV, ECV, 0b0010);

  private:
    enum Field {
      PARange = 0 * 4,
      ASIDBits = 1 * 4,
      BigEnd = 2 * 4,
      SNSMem = 3 * 4,
      BigEndEL0 = 4 * 4,
      TGran16 = 5 * 4,
      TGran64 = 6 * 4,
      TGran4 = 7 * 4,
      TGran16_2 = 8 * 4,
      TGran64_2 = 9 * 4,
      TGran4_2 = 10 * 4,
      ExS = 11 * 4,
      RES0 = 12 * 4,
      RES1 = 13 * 4,
      FGT = 14 * 4,
      ECV = 15 * 4,
    };
  };

  class MMFR2Reg final : public FeatureReg {
  public:
    FIELD_FETCHER(LSE2, AT, 0b0001);

  private:
    enum Field {
      CnP = 0 * 4,
      UAO = 1 * 4,
      LSM = 2 * 4,
      IESB = 3 * 4,
      VARange = 4 * 4,
      CCIDX = 5 * 4,
      NV = 6 * 4,
      ST = 7 * 4,
      AT = 8 * 4,
      IDS = 9 * 4,
      FWB = 10 * 4,
      RES0 = 11 * 4,
      TTL = 12 * 4,
      BBM = 13 * 4,
      EVT = 14 * 4,
      E0PD = 15 * 4,
    };
  };

  class ZFR0Reg final : public FeatureReg {
  public:
    FIELD_FETCHER(SVE2, SVEver, 0b0001);
    FIELD_FETCHER(SVE2_1, SVEver, 0b0010);

    FIELD_FETCHER(SVE_AES, AES, 0b0001);
    FIELD_FETCHER(SVE_PMULL128, AES, 0b0010);

    FIELD_FETCHER(SVE_BitPerm, BitPerm, 0b0001);

    FIELD_FETCHER(SVE_BF16, BF16, 0b0001);
    FIELD_FETCHER(SME_F64F64, BF16, 0b0010);

    FIELD_FETCHER(SVE_B16B16, B16B16, 0b0010);

    FIELD_FETCHER(SVE_SHA3, SHA3, 0b0001);

    FIELD_FETCHER(SVE_SM4, SM4, 0b0001);

    FIELD_FETCHER(SVE_I8MM, I8MM, 0b0001);

    FIELD_FETCHER(SVE_F32MM, F32MM, 0b0001);

    FIELD_FETCHER(SVE_F64MM, F64MM, 0b0001);

  private:
    enum Field {
      SVEver = 0 * 4,
      AES = 1 * 4,
      RES0 = 2 * 4,
      RES1 = 3 * 4,
      BitPerm = 4 * 4,
      BF16 = 5 * 4,
      B16B16 = 6 * 4,
      RES2 = 7 * 4,
      SHA3 = 8 * 4,
      RES3 = 9 * 4,
      SM4 = 10 * 4,
      I8MM = 11 * 4,
      RES4 = 12 * 4,
      F32MM = 13 * 4,
      F64MM = 14 * 4,
      RES5 = 15 * 4,
    };
  };

  class MMFR1Reg final : public FeatureReg {
  public:
    FIELD_FETCHER(AFP, AFP, 0b0001);

  private:
    enum Field {
      HAFDBS = 0 * 4,
      VMIDBits = 1 * 4,
      VH = 2 * 4,
      HPDS = 3 * 4,
      LO = 4 * 4,
      PAN = 5 * 4,
      SpecSEI = 6 * 4,
      XNX = 7 * 4,
      TWED = 8 * 4,
      ETS = 9 * 4,
      HCX = 10 * 4,
      AFP = 11 * 4,
      nTLBPA = 12 * 4,
      TIDCP1 = 13 * 4,
      CMOW = 14 * 4,
      ECBHB = 15 * 4,
    };
  };

  class ISAR2Reg final : public FeatureReg {
  public:
    FIELD_FETCHER(WFxt, WFxt, 0b0010);

    FIELD_FETCHER(RPRES, RPRES, 0b0001);

    FIELD_FETCHER(PACQARMA3, GPA3, 0b0001);

    FIELD_FETCHER(MOPS, MOPS, 0b0001);

    FIELD_FETCHER(HBC, BC, 0b0001);

    uint64_t PAC_Minor() const {
      return GetField(PAC_frac);
    }

    FIELD_FETCHER(CLRBHB, CLRBHB, 0b0001);

    FIELD_FETCHER(SYSREG128, SYSREG_128, 0b0001);

    FIELD_FETCHER(SYSINSTR128, SYSINSTR_128, 0b0001);

    FIELD_FETCHER(PRFMSLC, PRFMSLC, 0b0001);

    FIELD_FETCHER(RPRFM, RPRFM, 0b0001);

    FIELD_FETCHER(CSSC, CSSC, 0b0001);

  private:
    enum Field {
      WFxt = 0 * 4,
      RPRES = 1 * 4,
      GPA3 = 2 * 4,
      APA3 = 3 * 4,
      MOPS = 4 * 4,
      BC = 5 * 4,
      PAC_frac = 6 * 4,
      CLRBHB = 7 * 4,
      SYSREG_128 = 8 * 4,
      SYSINSTR_128 = 9 * 4,
      PRFMSLC = 10 * 4,
      RES0 = 11 * 4,
      RPRFM = 12 * 4,
      CSSC = 13 * 4,
      RES1 = 14 * 4,
      ATS1A = 15 * 4,
    };
  };

  class SVEVLReg final : public FeatureReg {};
#undef FIELD_FETCHER


  ISAR0Reg ISAR0;
  PFR0Reg PFR0;
  PFR1Reg PFR1;
  MIDRReg MIDR;
  ISAR1Reg ISAR1;
  MMFR0Reg MMFR0;
  ZFR0Reg ZFR0;
  MMFR2Reg MMFR2;
  MMFR1Reg MMFR1;
  ISAR2Reg ISAR2;
  DCZIDReg DCZID;
  SVEVLReg SVEVL;

  static_assert(FEXCore::ToUnderlying(Feature::MAX) < 128);
  static_assert((FEXCore::ToUnderlying(Feature::MAX) / (sizeof(uint64_t) * 8)) == 1);

  bool Supports(Feature feat) const {
    const size_t DWordSelect = FEXCore::ToUnderlying(feat) / (sizeof(uint64_t) * 8);
    const size_t BitSelect = FEXCore::ToUnderlying(feat) - (DWordSelect * (sizeof(uint64_t) * 8));
    return (FeatureBits[DWordSelect] >> BitSelect) & 1;
  }

  void RemoveFeature(Feature feat) {
    const size_t DWordSelect = FEXCore::ToUnderlying(feat) / (sizeof(uint64_t) * 8);
    const size_t BitSelect = FEXCore::ToUnderlying(feat) - (DWordSelect * (sizeof(uint64_t) * 8));
    FeatureBits[DWordSelect] &= ~(1ULL << BitSelect);
  }

  const DCZIDReg& GetDCZID() const {
    return DCZID;
  }

  uint64_t GetSVEVectorLengthInBits() const {
    return SVEVL.Get();
  }

protected:
  void FillFeatureFlags();

  uint64_t FeatureBits[(FEXCore::ToUnderlying(Feature::MAX) / (sizeof(uint64_t) * 8)) + 1] {};

  void SetFeature(Feature feat) {
    const size_t DWordSelect = FEXCore::ToUnderlying(feat) / (sizeof(uint64_t) * 8);
    const size_t BitSelect = FEXCore::ToUnderlying(feat) - (DWordSelect * (sizeof(uint64_t) * 8));
    FeatureBits[DWordSelect] |= 1ULL << BitSelect;
  }
};

void FillMIDRInformationViaLinux(FEXCore::HostFeatures* Features);

void FetchHostFeatures(FEX::CPUFeatures& Features, FEXCore::HostFeatures& HostFeatures, bool SupportsCacheMaintenanceOps, uint64_t CTR,
                       uint64_t MIDR);
FEXCore::HostFeatures FetchHostFeatures();
FEX::CPUFeatures GetCPUFeaturesFromIDRegisters();
} // namespace FEX
