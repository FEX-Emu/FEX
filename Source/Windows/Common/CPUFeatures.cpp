// SPDX-License-Identifier: MIT

#include "Common/CPUInfo.h"

#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/HostFeatures.h>
#include <FEXCore/fextl/fmt.h>

#include <windows.h>

#include "CPUFeatures.h"

namespace {

HKEY OpenProcessorKey(uint32_t Idx) {
  HKEY Out;
  auto Path = fextl::fmt::format("Hardware\\Description\\System\\CentralProcessor\\{}", Idx);
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, Path.c_str(), 0, KEY_READ, &Out)) {
    return nullptr;
  }
  return Out;
}

uint64_t ReadRegU64(HKEY Key, const char* Name) {
  uint64_t Value = 0;
  DWORD Size = sizeof(Value);
  RegGetValueA(Key, nullptr, Name, 0, nullptr, &Value, &Size);
  return Value;
}

} // namespace

namespace FEX::Windows {
class CPUFeaturesFromRegistry final : public FEX::CPUFeatures {
public:
  explicit CPUFeaturesFromRegistry(HKEY Key) {
    ISAR0.SetReg(ReadRegU64(Key, "CP 4030"));
    PFR0.SetReg(ReadRegU64(Key, "CP 4020"));
    PFR1.SetReg(ReadRegU64(Key, "CP 4021"));
    ISAR1.SetReg(ReadRegU64(Key, "CP 4031"));
    MMFR0.SetReg(ReadRegU64(Key, "CP 4038"));
    MMFR2.SetReg(ReadRegU64(Key, "CP 403A"));
    ZFR0.SetReg(ReadRegU64(Key, "CP 4024"));
    MMFR1.SetReg(ReadRegU64(Key, "CP 4039"));
    ISAR2.SetReg(ReadRegU64(Key, "CP 4032"));
    FillFeatureFlags();
  }
};

FEXCore::HostFeatures CPUFeatures::FetchHostFeatures(bool IsWine) {
  HKEY Key = OpenProcessorKey(0);
  if (!Key) {
    ERROR_AND_DIE_FMT("Couldn't detect CPU features");
  }

  CPUFeaturesFromRegistry Features(Key);

  uint64_t CTR = ReadRegU64(Key, "CP 5801");
  uint64_t MIDR = ReadRegU64(Key, "CP 4000");

  FEXCore::HostFeatures HostFeatures = {};

  for (uint32_t Idx = 0; Key; Key = OpenProcessorKey(++Idx)) {
    // Truncate to 32-bits, top 32-bits are all reserved in MIDR
    HostFeatures.CPUMIDRs.push_back(static_cast<uint32_t>(ReadRegU64(Key, "CP 4000")));
    RegCloseKey(Key);
  }

  FEX::FetchHostFeatures(Features, HostFeatures, !IsWine, CTR, MIDR);

  // Force-disable SVE until wine/windows gain support for SVE context save/restore
  HostFeatures.SupportsSVE128 = false;
  HostFeatures.SupportsSVE256 = false;

  HostFeatures.SupportsCPUIndexInTPIDRRO = !IsWine;
  return HostFeatures;
}

CPUFeatures::CPUFeatures(FEXCore::Context::Context& CTX) {
#ifdef ARCHITECTURE_arm64ec
  // Report as a 64-bit host for ARM64EC.
  CpuInfo.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
#else
  // Report as a 32-bit host for WoW64.
  CpuInfo.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
#endif

  // Baseline FEX feature-set
  CpuInfo.ProcessorFeatureBits = CPU_FEATURE_VME | CPU_FEATURE_TSC | CPU_FEATURE_CMOV | CPU_FEATURE_PGE | CPU_FEATURE_PSE | CPU_FEATURE_MTRR |
                                 CPU_FEATURE_CX8 | CPU_FEATURE_MMX | CPU_FEATURE_X86 | CPU_FEATURE_PAT | CPU_FEATURE_FXSR | CPU_FEATURE_SEP |
                                 CPU_FEATURE_SSE | CPU_FEATURE_3DNOW | CPU_FEATURE_SSE2 | CPU_FEATURE_SSE3 | CPU_FEATURE_CX128 |
                                 CPU_FEATURE_NX | CPU_FEATURE_SSSE3 | CPU_FEATURE_SSE41 | CPU_FEATURE_PAE | CPU_FEATURE_DAZ;

  // Features that require specific host CPU support
  const auto CPUIDResult01 = CTX.RunCPUIDFunction(0x01, 0);
  if (CPUIDResult01.ecx & (1 << 20)) {
    CpuInfo.ProcessorFeatureBits |= CPU_FEATURE_SSE42;
  }
  if (CPUIDResult01.ecx & (1 << 27)) {
    CpuInfo.ProcessorFeatureBits |= CPU_FEATURE_XSAVE;
  }
  if (CPUIDResult01.ecx & (1 << 28)) {
    CpuInfo.ProcessorFeatureBits |= CPU_FEATURE_AVX;
  }

  const auto CPUIDResult07 = CTX.RunCPUIDFunction(0x07, 0);
  if (CPUIDResult07.ebx & (1 << 5)) {
    CpuInfo.ProcessorFeatureBits |= CPU_FEATURE_AVX2;
  }

  const auto FamilyIdentifier = CPUIDResult01.eax;
  CpuInfo.ProcessorLevel = ((FamilyIdentifier >> 8) & 0xf) + ((FamilyIdentifier >> 20) & 0xff); // Family
  CpuInfo.ProcessorRevision = (FamilyIdentifier & 0xf0000) >> 4;                                // Extended Model
  CpuInfo.ProcessorRevision |= (FamilyIdentifier & 0xf0) << 4;                                  // Model
  CpuInfo.ProcessorRevision |= FamilyIdentifier & 0xf;                                          // Stepping
}

bool CPUFeatures::IsFeaturePresent(uint32_t Feature) {
  switch (Feature) {
  case PF_FLOATING_POINT_PRECISION_ERRATA: return FALSE;
  case PF_FLOATING_POINT_EMULATED: return FALSE;
  case PF_COMPARE_EXCHANGE_DOUBLE: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_CX8);
  case PF_MMX_INSTRUCTIONS_AVAILABLE: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_MMX);
  case PF_XMMI_INSTRUCTIONS_AVAILABLE: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_SSE);
  case PF_3DNOW_INSTRUCTIONS_AVAILABLE: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_3DNOW);
  case PF_RDTSC_INSTRUCTION_AVAILABLE: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_TSC);
  case PF_PAE_ENABLED: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_PAE);
  case PF_XMMI64_INSTRUCTIONS_AVAILABLE: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_SSE2);
  case PF_SSE3_INSTRUCTIONS_AVAILABLE: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_SSE3);
  case PF_SSSE3_INSTRUCTIONS_AVAILABLE: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_SSSE3);
  case PF_XSAVE_ENABLED: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_XSAVE);
  case PF_COMPARE_EXCHANGE128: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_CX128);
  case PF_SSE_DAZ_MODE_AVAILABLE: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_DAZ);
  case PF_NX_ENABLED: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_NX);
  case PF_SECOND_LEVEL_ADDRESS_TRANSLATION: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_2NDLEV);
  case PF_VIRT_FIRMWARE_ENABLED: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_VIRT);
  case PF_RDWRFSGSBASE_AVAILABLE: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_RDFS);
  case PF_FASTFAIL_AVAILABLE: return TRUE;
  case PF_SSE4_1_INSTRUCTIONS_AVAILABLE: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_SSE41);
  case PF_SSE4_2_INSTRUCTIONS_AVAILABLE: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_SSE42);
  case PF_AVX_INSTRUCTIONS_AVAILABLE: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_AVX);
  case PF_AVX2_INSTRUCTIONS_AVAILABLE: return !!(CpuInfo.ProcessorFeatureBits & CPU_FEATURE_AVX2);
  default: return false;
  }
}

void CPUFeatures::UpdateInformation(SYSTEM_CPU_INFORMATION* Info) {
  Info->ProcessorArchitecture = CpuInfo.ProcessorArchitecture;
  Info->ProcessorLevel = CpuInfo.ProcessorLevel;
  Info->ProcessorRevision = CpuInfo.ProcessorRevision;
  Info->ProcessorFeatureBits = CpuInfo.ProcessorFeatureBits;
}
} // namespace FEX::Windows
