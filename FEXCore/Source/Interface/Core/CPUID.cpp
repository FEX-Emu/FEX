// SPDX-License-Identifier: MIT
/*
$info$
tags: opcodes|cpuid
desc: Handles presented capability bits for guest cpu
$end_info$
*/

#include "Common/StringConv.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/CPUID.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CPUID.h>
#include <FEXCore/Core/HostFeatures.h>
#include <FEXCore/Utils/CPUInfo.h>
#include <FEXCore/Utils/FileLoading.h>
#include <FEXCore/fextl/string.h>
#include <FEXHeaderUtils/Syscalls.h>

#include "git_version.h"

#include <cstring>

namespace FEXCore {
namespace ProductNames {
#ifdef _M_ARM_64
  static const char ARM_UNKNOWN[] = "Unknown ARM CPU";
  static const char ARM_A57[] = "Cortex-A57";
  static const char ARM_A72[] = "Cortex-A72";
  static const char ARM_A73[] = "Cortex-A73";
  static const char ARM_A75[] = "Cortex-A75";
  static const char ARM_A76[] = "Cortex-A76";
  static const char ARM_A76AE[] = "Cortex-A76AE";
  static const char ARM_V1[] = "Neoverse V1";
  static const char ARM_V2[] = "Neoverse V2";
  static const char ARM_A77[] = "Cortex-A77";
  static const char ARM_A78[] = "Cortex-A78";
  static const char ARM_A78AE[] = "Cortex-A78AE";
  static const char ARM_A78C[] = "Cortex-A78C";
  static const char ARM_A710[] = "Cortex-A710";
  static const char ARM_A715[] = "Cortex-A715";
  static const char ARM_A720[] = "Cortex-A720";
  static const char ARM_X1[] = "Cortex-X1";
  static const char ARM_X1C[] = "Cortex-X1C";
  static const char ARM_X2[] = "Cortex-X2";
  static const char ARM_X3[] = "Cortex-X3";
  static const char ARM_X4[] = "Cortex-X4";
  static const char ARM_N1[] = "Neoverse N1";
  static const char ARM_N2[] = "Neoverse N2";
  static const char ARM_E1[] = "Neoverse E1";
  static const char ARM_A35[] = "Cortex-A35";
  static const char ARM_A53[] = "Cortex-A53";
  static const char ARM_A55[] = "Cortex-A55";
  static const char ARM_A65[] = "Cortex-A65";
  static const char ARM_A510[] = "Cortex-A510";
  static const char ARM_A520[] = "Cortex-A520";

  static const char ARM_Kryo200[] = "Kryo 2xx";
  static const char ARM_Kryo300[] = "Kryo 3xx";
  static const char ARM_Kryo400[] = "Kryo 4xx/5xx";

  static const char ARM_Kryo200S[] = "Kryo 2xx S";
  static const char ARM_Kryo300S[] = "Kryo 3xx S";
  static const char ARM_Kryo400S[] = "Kryo 4xx/5xx S";

  static const char ARM_Denver[] = "Nvidia Denver";
  static const char ARM_Carmel[] = "Nvidia Carmel";

  static const char ARM_Firestorm[] = "Apple Firestorm";
  static const char ARM_Icestorm[] = "Apple Icestorm";

  static const char ARM_ORYON_1[] = "Oryon-1";
#else
#endif
} // namespace ProductNames

static uint32_t GetCPUID() {
  uint32_t CPU {};
  FHU::Syscalls::getcpu(&CPU, nullptr);
  return CPU;
}

#ifdef CPUID_AMD
constexpr uint32_t FAMILY_IDENTIFIER = 0 |          // Stepping
                                       (0xA << 4) | // Model
                                       (0xF << 8) | // Family ID
                                       (0 << 12) |  // Processor type
                                       (0 << 16) |  // Extended model ID
                                       (1 << 20);   // Extended family ID
#else
constexpr uint32_t FAMILY_IDENTIFIER = 0 |          // Stepping
                                       (0x7 << 4) | // Model
                                       (0x6 << 8) | // Family ID
                                       (0 << 12) |  // Processor type
                                       (1 << 16) |  // Extended model ID
                                       (0x0 << 20); // Extended family ID
#endif

#ifdef _M_ARM_64
uint32_t GetCycleCounterFrequency() {
  uint64_t Result {};
  __asm("mrs %[Res], CNTFRQ_EL0" : [Res] "=r"(Result));
  return Result;
}

void CPUIDEmu::SetupHostHybridFlag() {
  PerCPUData.resize(Cores);

  uint64_t MIDR {};
  for (size_t i = 0; i < Cores; ++i) {
    std::error_code ec {};
    fextl::string MIDRPath = fextl::fmt::format("/sys/devices/system/cpu/cpu{}/regs/identification/midr_el1", i);

    std::array<char, 18> Data;
    // Needs to be a fixed size since depending on kernel it will try to read a full page of data and fail
    // Only read 18 bytes for a 64bit value prefixed with 0x
    if (FEXCore::FileLoading::LoadFileToBuffer(MIDRPath, Data) == sizeof(Data)) {
      uint64_t NewMIDR {};
      std::string_view MIDRView(Data.data(), sizeof(Data));
      if (FEXCore::StrConv::Conv(MIDRView, &NewMIDR)) {
        if (MIDR != 0 && MIDR != NewMIDR) {
          // CPU mismatch, claim hybrid
          Hybrid = true;
        }

        // Truncate to 32-bits, top 32-bits are all reserved in MIDR
        PerCPUData[i].ProductName = ProductNames::ARM_UNKNOWN;
        PerCPUData[i].MIDR = NewMIDR;
        MIDR = NewMIDR;
      }
    }
  }

  struct CPUMIDR {
    uint8_t Implementer;
    uint16_t Part;
    bool DefaultBig; // Defaults to a big core
    const char* ProductName {};
  };

  // CPU priority order
  // This is mostly arbitrary but will sort by some sort of CPU priority by performance
  // Relative list so things they will commonly end up in big.little configurations sort of relate
  static constexpr std::array<CPUMIDR, 43> CPUMIDRs = {{
    // Typically big CPU cores
    {0x51, 0x001, 1, ProductNames::ARM_ORYON_1}, // Qualcomm Oryon-1

    {0x61, 0x023, 1, ProductNames::ARM_Firestorm}, // Apple M1 Firestorm

    {0x41, 0xd82, 1, ProductNames::ARM_X4},      // X4
    {0x41, 0xd81, 1, ProductNames::ARM_A720},    // A720
    {0x41, 0xd4e, 1, ProductNames::ARM_X3},      // X3
    {0x41, 0xd4d, 1, ProductNames::ARM_A715},    // A715
    {0x41, 0xd4f, 1, ProductNames::ARM_V2},      // V2
    {0x41, 0xd49, 1, ProductNames::ARM_N2},      // N2
    {0x41, 0xd4b, 1, ProductNames::ARM_A78C},    // A78C
    {0x41, 0xd4a, 1, ProductNames::ARM_E1},      // E1
    {0x41, 0xd49, 1, ProductNames::ARM_N2},      // N2
    {0x41, 0xd48, 1, ProductNames::ARM_X2},      // X2
    {0x41, 0xd47, 1, ProductNames::ARM_A710},    // A710
    {0x41, 0xd4C, 1, ProductNames::ARM_X1C},     // X1C
    {0x41, 0xd44, 1, ProductNames::ARM_X1},      // X1
    {0x41, 0xd42, 1, ProductNames::ARM_A78AE},   // A78AE
    {0x41, 0xd41, 1, ProductNames::ARM_A78},     // A78
    {0x41, 0xd40, 1, ProductNames::ARM_V1},      // V1
    {0x41, 0xd0e, 1, ProductNames::ARM_A76AE},   // A76AE
    {0x41, 0xd0d, 1, ProductNames::ARM_A77},     // A77
    {0x41, 0xd0c, 1, ProductNames::ARM_N1},      // N1
    {0x41, 0xd0b, 1, ProductNames::ARM_A76},     // A76
    {0x51, 0x804, 1, ProductNames::ARM_Kryo400}, // Kryo 4xx Gold (A76 based)
    {0x41, 0xd0a, 1, ProductNames::ARM_A75},     // A75
    {0x51, 0x802, 1, ProductNames::ARM_Kryo300}, // Kryo 3xx Gold (A75 based)
    {0x41, 0xd09, 1, ProductNames::ARM_A73},     // A73
    {0x51, 0x800, 1, ProductNames::ARM_Kryo200}, // Kryo 2xx Gold (A73 based)
    {0x41, 0xd08, 1, ProductNames::ARM_A72},     // A72

    {0x4e, 0x004, 1, ProductNames::ARM_Carmel}, // Carmel

    // Denver rated above A57 to match TX2 weirdness
    {0x4e, 0x003, 1, ProductNames::ARM_Denver}, // Denver

    {0x41, 0xd07, 1, ProductNames::ARM_A57}, // A57

    // Typically Little CPU cores
    {0x61, 0x022, 0, ProductNames::ARM_Icestorm}, // Apple M1 Icestorm
    {0x41, 0xd80, 0, ProductNames::ARM_A520},     // A520
    {0x41, 0xd46, 0, ProductNames::ARM_A510},     // A510
    {0x41, 0xd06, 0, ProductNames::ARM_A65},      // A65
    {0x41, 0xd05, 0, ProductNames::ARM_A55},      // A55
    {0x51, 0x805, 0, ProductNames::ARM_Kryo400S}, // Kryo 4xx/5xx Silver (A55 based)
    {0x51, 0x803, 0, ProductNames::ARM_Kryo300S}, // Kryo 3xx Silver (A55 based)
    {0x41, 0xd03, 0, ProductNames::ARM_A53},      // A53
    {0x51, 0x801, 0, ProductNames::ARM_Kryo200S}, // Kryo 2xx Silver (A53 based)
    {0x41, 0xd04, 0, ProductNames::ARM_A35},      // A35

    {0x41, 0, 0, ProductNames::ARM_UNKNOWN}, // Invalid CPU or Apple CPU inside Parallels VM
    {0x0, 0, 0, ProductNames::ARM_UNKNOWN},  // Invalid starting point is lowest ranked
  }};

  auto FindDefinedMIDR = [](uint32_t MIDR) -> const CPUMIDR* {
    uint8_t Implementer = MIDR >> 24;
    uint16_t Part = (MIDR >> 4) & 0xFFF;

    for (auto& MIDROption : CPUMIDRs) {
      if (MIDROption.Implementer == Implementer && MIDROption.Part == Part) {
        return &MIDROption;
      }
    }

    return nullptr;
  };

  if (Hybrid) {
    // Walk the MIDRs and calculate big little designs
    fextl::vector<const CPUMIDR*> BigCores;
    fextl::vector<const CPUMIDR*> LittleCores;

    // Separate CPU cores out to big or little selected
    for (size_t i = 0; i < Cores; ++i) {
      uint32_t MIDR = PerCPUData[i].MIDR;
      auto MIDROption = FindDefinedMIDR(MIDR);
      if (MIDROption) {
        // Found one
        if (MIDROption->DefaultBig) {
          BigCores.emplace_back(MIDROption);
        } else {
          LittleCores.emplace_back(MIDROption);
        }
      } else {
        // If we didn't insert this MIDR then claim it is a little core.
        LittleCores.emplace_back(&CPUMIDRs.back());
      }
    }

    if (LittleCores.empty()) {
      // If we only ended up with big cores then we need to move some to be little cores
      uint32_t LowestMIDR = ~0U;
      uint32_t LowestMIDRIdx = 0;
      // Walk all the big cores
      for (size_t i = 0; i < BigCores.size(); ++i) {
        uint8_t Implementer = BigCores[i]->Implementer;
        uint16_t Part = BigCores[i]->Part;

        // Walk our list of CPUMIDRs to find the most little core
        for (size_t j = LowestMIDRIdx; j < CPUMIDRs.size(); ++j) {
          auto& MIDROption = CPUMIDRs[i];
          if ((MIDROption.Implementer == Implementer && MIDROption.Part == Part) || (MIDROption.Implementer == 0 && MIDROption.Part == 0)) {

            LowestMIDRIdx = j;
            LowestMIDR = MIDR;
            break;
          }
        }
      }

      // Now we WILL have found a big core to demote to little status
      // Demote them
      std::erase_if(BigCores, [&LittleCores, LowestMIDR](auto* Entry) {
        // Demote by erase copy to little array
        uint8_t Implementer = LowestMIDR >> 24;
        uint16_t Part = (LowestMIDR >> 4) & 0xFFF;

        if (Entry->Implementer == Implementer && Entry->Part == Part) {
          // Add it to the BigCore list
          LittleCores.emplace_back(Entry);
          return true;
        }
        return false;
      });
    }

    if (BigCores.empty()) {
      // We never found a CPU core we understand
      // Grab the first core, consider it as little, move everything else to Big
      uint32_t LittleMIDR = PerCPUData[0].MIDR;
      // Now walk the little cores and move them to Big if they don't match
      std::erase_if(LittleCores, [&BigCores, LittleMIDR](auto* Entry) {
        // You're promoted now
        uint8_t Implementer = LittleMIDR >> 24;
        uint16_t Part = (LittleMIDR >> 4) & 0xFFF;

        if (Entry->Implementer != Implementer || Entry->Part != Part) {
          // Add it to the BigCore list
          BigCores.emplace_back(Entry);
          return true;
        }
        return false;
      });
    }

    // Now walk the per CPU data one more time and set if it is big or little
    for (auto& Data : PerCPUData) {
      uint8_t Implementer = Data.MIDR >> 24;
      uint16_t Part = (Data.MIDR >> 4) & 0xFFF;

      bool FoundBig {};
      const CPUMIDR* MIDR {};
      for (auto Big : BigCores) {
        if (Big->Implementer == Implementer && Big->Part == Part) {
          FoundBig = true;
          MIDR = Big;
          break;
        }
      }

      if (!FoundBig) {
        for (auto Little : LittleCores) {
          if (Little->Implementer == Implementer && Little->Part == Part) {
            MIDR = Little;
            break;
          }
        }
      }

      Data.IsBig = FoundBig;
      if (MIDR) {
        Data.ProductName = MIDR->ProductName ?: ProductNames::ARM_UNKNOWN;
      } else {
        Data.ProductName = ProductNames::ARM_UNKNOWN;
      }
    }
  } else {
    // If we aren't hybrid then just claim everything is big
    for (size_t i = 0; i < Cores; ++i) {
      uint32_t MIDR = PerCPUData[i].MIDR;
      auto MIDROption = FindDefinedMIDR(MIDR);

      PerCPUData[i].IsBig = true;
      if (MIDROption) {
        PerCPUData[i].ProductName = MIDROption->ProductName ?: ProductNames::ARM_UNKNOWN;
      } else {
        PerCPUData[i].ProductName = ProductNames::ARM_UNKNOWN;
      }
    }
  }
}

#else
uint32_t GetCycleCounterFrequency() {
  return 0;
}

void CPUIDEmu::SetupHostHybridFlag() {}

#endif


void CPUIDEmu::SetupFeatures() {
  // TODO: Enable once AVX is supported.
  if (false && CTX->HostFeatures.SupportsAVX) {
    XCR0 |= XCR0_AVX;
  }

  // Override features if the user has specifically called for it.
  FEX_CONFIG_OPT(CPUIDFeatures, CPUID);
  if (!CPUIDFeatures()) {
    // Early exit if no features are overriden.
    return;
  }

#define ENABLE_DISABLE_OPTION(FeatureName, name, enum_name)                                                                        \
  do {                                                                                                                             \
    const bool Disable##name = (CPUIDFeatures() & FEXCore::Config::CPUID::DISABLE##enum_name) != 0;                                \
    const bool Enable##name = (CPUIDFeatures() & FEXCore::Config::CPUID::ENABLE##enum_name) != 0;                                  \
    LogMan::Throw::AFmt(!(Disable##name && Enable##name), "Disabling and Enabling CPU feature (" #name ") is mutually exclusive"); \
    const bool AlreadyEnabled = Features.FeatureName;                                                                              \
    const bool Result = (AlreadyEnabled | Enable##name) & !Disable##name;                                                          \
    Features.FeatureName = Result;                                                                                                 \
  } while (0)

  ENABLE_DISABLE_OPTION(SHA, SHA, SHA);
#undef ENABLE_DISABLE_OPTION
}

FEXCore::CPUID::FunctionResults CPUIDEmu::Function_0h(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};

  // EBX, EDX, ECX become the manufacturer id string
#ifdef CPUID_AMD
  Res.eax = 0x0D; // Let's say we are a Zen+
  Res.ebx = CPUID_VENDOR_AMD1;
  Res.edx = CPUID_VENDOR_AMD2;
  Res.ecx = CPUID_VENDOR_AMD3;
#else
  Res.eax = 0x16; // Let's say we are a Skylake
  Res.ebx = CPUID_VENDOR_INTEL1;
  Res.edx = CPUID_VENDOR_INTEL2;
  Res.ecx = CPUID_VENDOR_INTEL3;
#endif
  return Res;
}

// Processor Info and Features bits
FEXCore::CPUID::FunctionResults CPUIDEmu::Function_01h(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};

  // Hypervisor bit is normally set but some applications have issues with it.
  uint32_t Hypervisor = HideHypervisorBit() ? 0 : 1;

  Res.eax = FAMILY_IDENTIFIER;

  Res.ebx = 0 |             // Brand index
            (8 << 8) |      // Cache line size in bytes
            (Cores << 16) | // Number of addressable IDs for the logical cores in the physical CPU
            (0 << 24);      // Local APIC ID

  Res.ecx = (1 << 0) |                                      // SSE3
            (CTX->HostFeatures.SupportsPMULL_128Bit << 1) | // PCLMULQDQ
            (1 << 2) |                                      // DS area supports 64bit layout
            (1 << 3) |                                      // MWait
            (0 << 4) |                                      // DS-CPL
            (0 << 5) |                                      // VMX
            (0 << 6) |                                      // SMX
            (0 << 7) |                                      // Intel SpeedStep
            (1 << 8) |                                      // Thermal Monitor 2
            (1 << 9) |                                      // SSSE3
            (0 << 10) |                                     // L1 context ID
            (0 << 11) |                                     // Silicon debug
            (0 << 12) |                                     // FMA3
            (1 << 13) |                                     // CMPXCHG16B
            (0 << 14) |                                     // xTPR update control
            (0 << 15) |                                     // Perfmon and debug capability
            (0 << 16) |                                     // Reserved
            (0 << 17) |                                     // Process-context identifiers
            (0 << 18) |                                     // Prefetching from memory mapped device
            (1 << 19) |                                     // SSE4.1
            (CTX->HostFeatures.SupportsCRC << 20) |         // SSE4.2
            (0 << 21) |                                     // X2APIC
            (1 << 22) |                                     // MOVBE
            (1 << 23) |                                     // POPCNT
            (0 << 24) |                                     // APIC TSC-Deadline
            (CTX->HostFeatures.SupportsAES << 25) |         // AES
            (SupportsAVX() << 26) |                         // XSAVE
            (SupportsAVX() << 27) |                         // OSXSAVE
            (SupportsAVX() << 28) |                         // AVX
            (SupportsAVX() << 29) |                         // F16C
            (CTX->HostFeatures.SupportsRAND << 30) |        // RDRAND
            (Hypervisor << 31);

  Res.edx = (1 << 0) |  // FPU
            (1 << 1) |  // Virtual 8086 mode enhancements
            (0 << 2) |  // Debugging extensions
            (0 << 3) |  // Page size extension
            (1 << 4) |  // RDTSC supported
            (1 << 5) |  // MSR supported
            (1 << 6) |  // PAE
            (1 << 7) |  // Machine Check exception
            (1 << 8) |  // CMPXCHG8B
            (1 << 9) |  // APIC on-chip
            (0 << 10) | // Reserved
            (1 << 11) | // SYSENTER/SYSEXIT
            (1 << 12) | // Memory Type Range registers, MTRRs are supported
            (1 << 13) | // Page Global bit
            (1 << 14) | // Machine Check architecture
            (1 << 15) | // CMOV
            (1 << 16) | // Page Attribute Table
            (1 << 17) | // 36bit page size extension
            (0 << 18) | // Processor serial number
            (1 << 19) | // CLFLUSH
            (0 << 20) | // Reserved
            (0 << 21) | // Debug store
            (0 << 22) | // Thermal monitor and software controled clock
            (1 << 23) | // MMX
            (1 << 24) | // FXSAVE/FXRSTOR
            (1 << 25) | // SSE
            (1 << 26) | // SSE2
            (0 << 27) | // Self Snoop
            (1 << 28) | // Max APIC IDs reserved field is valid
            (1 << 29) | // Thermal monitor
            (0 << 30) | // Reserved
            (0 << 31);  // Pending break enable
  return Res;
}

// 2: Cache and TLB information
FEXCore::CPUID::FunctionResults CPUIDEmu::Function_02h(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};

  // returns default values from i7 model 1Ah
  Res.eax = 0x1 | // Number of iterations needed for all descriptors
            (0x5A << 8) | (0x03 << 16) | (0x55 << 24);

  Res.ebx = 0xE4 | (0xB2 << 8) | (0xF0 << 16) | (0 << 24);

  Res.ecx = 0; // null descriptors

  Res.edx = 0x2C | (0x21 << 8) | (0xCA << 16) | (0x09 << 24);

  return Res;
}

// 4: Deterministic cache parameters for each level
FEXCore::CPUID::FunctionResults CPUIDEmu::Function_04h(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};
  constexpr uint32_t CacheType_Data = 1;
  constexpr uint32_t CacheType_Instruction = 2;
  constexpr uint32_t CacheType_Unified = 3;

  if (Leaf == 0) {
    // Report L1D
    uint32_t CoreCount = Cores - 1;

    Res.eax = CacheType_Data |   // Cache type
              (0b001 << 5) |     // Cache level
              (1 << 8) |         // Self initializing cache level
              (0 << 9) |         // Fully associative
              (0 << 14) |        // Maximum number of addressable IDs for logical processors sharing this cache (With SMT this would be 1)
              (CoreCount << 26); // Maximum number of addressable IDs for processor cores in the physical package

    Res.ebx = (63 << 0) | // Line Size - 1 : Claiming 64 byte
              (0 << 12) | // Physical Line partitions
              (7 << 22);  // Associativity - 1 : Claiming 8 way

    // 32KB
    Res.ecx = 63; // Number of sets - 1 : Claiming 64 sets

    Res.edx = (0 << 0) | // Write-back invalidate
              (0 << 1) | // Cache inclusiveness - Includes lower caches
              (0 << 2);  // Complex cache indexing - 0: Direct, 1: Complex
  } else if (Leaf == 1) {
    // Report L1I
    uint32_t CoreCount = Cores - 1;

    Res.eax = CacheType_Instruction | // Cache type
              (0b001 << 5) |          // Cache level
              (1 << 8) |              // Self initializing cache level
              (0 << 9) |              // Fully associative
              (0 << 14) |        // Maximum number of addressable IDs for logical processors sharing this cache (With SMT this would be 1)
              (CoreCount << 26); // Maximum number of addressable IDs for processor cores in the physical package

    Res.ebx = (63 << 0) | // Line Size - 1 : Claiming 64 byte
              (0 << 12) | // Physical Line partitions
              (7 << 22);  // Associativity - 1 : Claiming 8 way

    // 32KB
    Res.ecx = 63; // Number of sets - 1 : Claiming 64 sets

    Res.edx = (0 << 0) | // Write-back invalidate
              (0 << 1) | // Cache inclusiveness - Includes lower caches
              (0 << 2);  // Complex cache indexing - 0: Direct, 1: Complex
  } else if (Leaf == 2) {
    // Report L2
    uint32_t CoreCount = Cores - 1;

    Res.eax = CacheType_Unified | // Cache type
              (0b010 << 5) |      // Cache level
              (1 << 8) |          // Self initializing cache level
              (0 << 9) |          // Fully associative
              (0 << 14) |         // Maximum number of addressable IDs for logical processors sharing this cache
              (CoreCount << 26);  // Maximum number of addressable IDs for processor cores in the physical package

    Res.ebx = (63 << 0) | // Line Size - 1 : Claiming 64 byte
              (0 << 12) | // Physical Line partitions
              (7 << 22);  // Associativity - 1 : Claiming 8 way

    // 512KB
    Res.ecx = 0x3FF; // Number of sets - 1 : Claiming 1024 sets

    Res.edx = (0 << 0) | // Write-back invalidate
              (0 << 1) | // Cache inclusiveness - Includes lower caches
              (0 << 2);  // Complex cache indexing - 0: Direct, 1: Complex
  } else if (Leaf == 3) {
    // Report L3
    uint32_t CoreCount = Cores - 1;

    Res.eax = CacheType_Unified | // Cache type
              (0b011 << 5) |      // Cache level
              (1 << 8) |          // Self initializing cache level
              (0 << 9) |          // Fully associative
              (CoreCount << 14) | // Maximum number of addressable IDs for logical processors sharing this cache
              (CoreCount << 26);  // Maximum number of addressable IDs for processor cores in the physical package

    Res.ebx = (63 << 0) | // Line Size - 1 : Claiming 64 byte
              (0 << 12) | // Physical Line partitions
              (7 << 22);  // Associativity - 1 : Claiming 8 way

    // 8MB
    Res.ecx = 0x4000; // Number of sets - 1 : Claiming 16384 sets

    Res.edx = (0 << 0) | // Write-back invalidate
              (0 << 1) | // Cache inclusiveness - Includes lower caches
              (1 << 2);  // Complex cache indexing - 0: Direct, 1: Complex
  }

  return Res;
}

FEXCore::CPUID::FunctionResults CPUIDEmu::Function_06h(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};
  Res.eax = (1 << 2); // Always running APIC
  Res.ecx = (0 << 3); // Intel performance energy bias preference (EPB)
  return Res;
}

FEXCore::CPUID::FunctionResults CPUIDEmu::Function_07h(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};
  if (Leaf == 0) {
    // Disable Enhanced REP MOVS when TSO is enabled.
    // vcruntime140 memmove will use `rep movsb` in this case which completely destroys perf in Hades(appId 1145360)
    // This is due to LRCPC performance on Cortex being abysmal.
    // Only enable EnhancedREPMOVS if SoftwareTSO isn't required OR if MemcpySetTSO is not enabled.
    const uint32_t SupportsEnhancedREPMOVS = CTX->SoftwareTSORequired() == false || MemcpySetTSOEnabled() == false;
    const uint32_t SupportsVPCLMULQDQ = CTX->HostFeatures.SupportsPMULL_128Bit && SupportsAVX();

    // Number of subfunctions
    Res.eax = 0x0;
    Res.ebx = (1 << 0) |                               // FS/GS support
              (0 << 1) |                               // TSC adjust MSR
              (0 << 2) |                               // SGX
              (SupportsAVX() << 3) |                   // BMI1
              (0 << 4) |                               // Intel Hardware Lock Elison
              (0 << 5) |                               // AVX2 support
              (1 << 6) |                               // FPU data pointer updated only on exception
              (1 << 7) |                               // SMEP support
              (SupportsAVX() << 8) |                   // BMI2
              (SupportsEnhancedREPMOVS << 9) |         // Enhanced REP MOVSB/STOSB
              (1 << 10) |                              // INVPCID for system software control of process-context
              (0 << 11) |                              // Restricted transactional memory
              (0 << 12) |                              // Intel resource directory technology Monitoring
              (1 << 13) |                              // Deprecates FPU CS and DS
              (0 << 14) |                              // Intel MPX
              (0 << 15) |                              // Intel Resource Directory Technology Allocation
              (0 << 16) |                              // Reserved
              (0 << 17) |                              // Reserved
              (CTX->HostFeatures.SupportsRAND << 18) | // RDSEED
              (1 << 19) |                              // ADCX and ADOX instructions
              (0 << 20) |                              // SMAP Supervisor mode access prevention and CLAC/STAC instructions
              (0 << 21) |                              // Reserved
              (0 << 22) |                              // Reserved
              (1 << 23) |                              // CLFLUSHOPT instruction
              (CTX->HostFeatures.SupportsCLWB << 24) | // CLWB instruction
              (0 << 25) |                              // Intel processor trace
              (0 << 26) |                              // Reserved
              (0 << 27) |                              // Reserved
              (0 << 28) |                              // Reserved
              (Features.SHA << 29) |                   // SHA instructions
              (0 << 30) |                              // Reserved
              (0 << 31);                               // Reserved

    Res.ecx = (1 << 0) |                                // PREFETCHWT1
              (0 << 1) |                                // AVX512VBMI
              (0 << 2) |                                // Usermode instruction prevention
              (0 << 3) |                                // Protection keys for user mode pages
              (0 << 4) |                                // OS protection keys
              (0 << 5) |                                // waitpkg
              (0 << 6) |                                // AVX512_VBMI2
              (0 << 7) |                                // CET shadow stack
              (0 << 8) |                                // GFNI
              (CTX->HostFeatures.SupportsAES256 << 9) | // VAES
              (SupportsVPCLMULQDQ << 10) |              // VPCLMULQDQ
              (0 << 11) |                               // AVX512_VNNI
              (0 << 12) |                               // AVX512_BITALG
              (0 << 13) |                               // Intel Total Memory Encryption
              (0 << 14) |                               // AVX512_VPOPCNTDQ
              (0 << 15) |                               // Reserved
              (0 << 16) |                               // 5 Level page tables
              (0 << 17) |                               // MPX MAWAU
              (0 << 18) |                               // MPX MAWAU
              (0 << 19) |                               // MPX MAWAU
              (0 << 20) |                               // MPX MAWAU
              (0 << 21) |                               // MPX MAWAU
              (1 << 22) |                               // RDPID Read Processor ID
              (0 << 23) |                               // Reserved
              (0 << 24) |                               // Reserved
              (0 << 25) |                               // CLDEMOTE
              (0 << 26) |                               // Reserved
              (0 << 27) |                               // MOVDIRI
              (0 << 28) |                               // MOVDIR64B
              (0 << 29) |                               // Reserved
              (0 << 30) |                               // SGX Launch configuration
              (0 << 31);                                // Reserved

    Res.edx = (0 << 0) |                   // Reserved
              (0 << 1) |                   // Reserved
              (0 << 2) |                   // AVX512_4VNNIW
              (0 << 3) |                   // AVX512_4FMAPS
              (1 << 4) |                   // Fast Short Rep Mov
              (0 << 5) |                   // Reserved
              (0 << 6) |                   // Reserved
              (0 << 7) |                   // Reserved
              (0 << 8) |                   // AVX512_VP2INTERSECT
              (0 << 9) |                   // SRBDS_CTRL (Special Register Buffer Data Sampling Mitigations)
              (0 << 10) |                  // VERW clears CPU buffers
              (0 << 11) |                  // Reserved
              (0 << 12) |                  // Reserved
              (0 << 13) |                  // TSX Force Abort (TSX will force abort if attempted)
              (0 << 14) |                  // SERIALIZE instruction
              ((Hybrid ? 1U : 0U) << 15) | // Hybrid
              (0 << 16) |                  // TSXLDTRK (TSX Suspend load address tracking) - Allows untracked memory loads inside TSX region
              (0 << 17) |                  // Reserved
              (0 << 18) |                  // Intel PCONFIG
              (0 << 19) |                  // Intel Architectural LBR
              (0 << 20) |                  // Intel CET
              (0 << 21) |                  // Reserved
              (0 << 22) |                  // AMX-BF16 - Tile computation on bfloat16
              (0 << 23) |                  // AVX512_FP16 - FP16 AVX512 instructions
              (0 << 24) |                  // AMX-tile - If AMX is implemented
              (0 << 25) |                  // AMX-int8 - AMX on 8-bit integers
              (0 << 26) |                  // IBRS_IBPB - Speculation control
              (0 << 27) |                  // STIBP - Single Thread Indirect Branch Predictor, Part of IBC
              (0 << 28) |                  // L1D Flush
              (0 << 29) |                  // Arch capabilities - Speculative side channel mitigations
              (0 << 30) |                  // Arch capabilities - MSR module specific
              (0 << 31);                   // SSBD - Speculative Store Bypass Disable
  }

  return Res;
}

FEXCore::CPUID::FunctionResults CPUIDEmu::Function_0Dh(uint32_t Leaf) const {
  // Leaf 0
  FEXCore::CPUID::FunctionResults Res {};

  uint32_t XFeatureSupportedSizeMax = SupportsAVX() ? 0x0000'0340 : 0x0000'0240; // XFeatureEnabledSizeMax: Legacy Header + FPU/SSE + AVX
  if (Leaf == 0) {
    // XFeatureSupportedMask[31:0]
    Res.eax = (1 << 0) |             // X87 support
              (1 << 1) |             // 128-bit SSE support
              (SupportsAVX() << 2) | // 256-bit AVX support
              (0b00 << 3) |          // MPX State
              (0b000 << 5) |         // AVX-512 state
              (0 << 8) |             // "Used for IA32_XSS" ... Used for what?
              (0 << 9);              // PKRU state

    // EBX and ECX doesn't need to match if a feature is supported but not enabled
    Res.ebx = XFeatureSupportedSizeMax;
    Res.ecx = XFeatureSupportedSizeMax; // XFeatureSupportedSizeMax: Size in bytes of XSAVE/XRSTOR area

    // XFeatureSupportedMask[63:32]
    Res.edx = 0; // Upper 32-bits of XFeatureSupportedMask
  } else if (Leaf == 1) {
    Res.eax = (0 << 0) | // XSAVEOPT
              (0 << 1) | // XSAVEC (and XRSTOR)
              (0 << 2) | // XGETBV - XGETBV with ECX=1 supported
              (0 << 3);  // XSAVES - XSAVES, XRSTORS, and IA32_XSS supported

    // Same information as Leaf 0 for ebx
    Res.ebx = XFeatureSupportedSizeMax;

    // Lower supported 32bits of IA32_XSS MSR. IA32_XSS[n] can only be set to 1 if ECX[n] is 1
    Res.ecx = (0b0000'0000 << 0) | // Used for XCR0
              (0 << 8) |           // PT state
              (0 << 9);            // Used for XCR0

    // Upper supported 32bits of IA32_XSS MSR. IA32_XSS[n+32] can only be set to 1 if EDX[n] is 1
    // Entirely reserved atm
    Res.edx = 0;
  } else if (Leaf == 2) {
    Res.eax = SupportsAVX() ? 0x0000'0100 : 0; // YmmSaveStateSize
    Res.ebx = SupportsAVX() ? 0x0000'0240 : 0; // YmmSaveStateOffset

    // Reserved
    Res.ecx = 0;
    Res.edx = 0;
  }
  return Res;
}

FEXCore::CPUID::FunctionResults CPUIDEmu::Function_15h(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};
  // TSC frequency = ECX * EBX / EAX
  uint32_t FrequencyHz = GetCycleCounterFrequency();
  if (FrequencyHz) {
    Res.eax = 1;
    Res.ebx = CTX->Config.SmallTSCScale() ? FEXCore::Context::TSC_SCALE : 1;
    Res.ecx = FrequencyHz;
  }
  return Res;
}

FEXCore::CPUID::FunctionResults CPUIDEmu::Function_1Ah(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};
  if (Hybrid) {
    uint32_t CPU = GetCPUID();
    auto& Data = PerCPUData[CPU];
    // 0x40 is a big CPU
    // 0x20 is a little CPU
    Res.eax |= (Data.IsBig ? 0x40 : 0x20) << 24;
  }
  return Res;
}

// Hypervisor CPUID information leaf
FEXCore::CPUID::FunctionResults CPUIDEmu::Function_4000_0000h(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};
  // Maximum supported hypervisor leafs
  // We only expose the information leaf
  //
  // Common courtesy to follow VMWare's "Hypervisor CPUID Interface proposal"
  // 4000_0000h - Information leaf. Advertising to the software which hypervisor this is
  // 4000_0001h - 4000_000Fh - Hypervisor specific leafs. FEX can use these for anything
  // 4000_0010h - 4000_00FFh - "Generic Leafs" - Try not to overwrite, other hypervisors might expect information in these
  //
  // CPUID documentation information:
  // 4000_0000h - 4FFF_FFFFh - No existing or future CPU will return information in this range
  // Reserved entirely for VMs to do whatever they want.
  Res.eax = 0x40000001;

  // EBX, EDX, ECX become the hypervisor ID signature
  constexpr static char HypervisorID[12] = "FEXIFEXIEMU";
  memcpy(&Res.ebx, HypervisorID, sizeof(HypervisorID));
  return Res;
}

constexpr std::array<char, std::char_traits<char>::length(GIT_DESCRIBE_STRING) + 1> GitString = {GIT_DESCRIBE_STRING};
static_assert(GitString.size() < 32);

// Hypervisor CPUID information leaf
FEXCore::CPUID::FunctionResults CPUIDEmu::Function_4000_0001h(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};
  constexpr uint32_t MaximumSubLeafNumber = 2;
  if (Leaf == 0) {
    // EAX[3:0] Is the host architecture that FEX is running under
#ifdef _M_X86_64
    // EAX[3:0] = 1 = x86_64 host architecture
    Res.eax |= 0b0001;
#elif defined(_M_ARM_64)
    // EAX[3:0] = 2 = AArch64 host architecture
    Res.eax |= 0b0010;
#else
    // EAX[3:0] = 0 = Unknown architecture
#endif

    // EAX[15:4] = Reserved

    // EAX[31:16] = Maximum sub-leaf value.
    Res.eax |= MaximumSubLeafNumber << 16;
  } else if (Leaf == 1) {
    memcpy(&Res, GitString.data(), std::min<size_t>(GitString.size(), sizeof(FEXCore::CPUID::FunctionResults)));
  } else if (Leaf == 2) {
    memcpy(&Res, GitString.data() + 16, std::min<size_t>(std::max<ssize_t>(0, GitString.size() - 16), sizeof(FEXCore::CPUID::FunctionResults)));
  }

  return Res;
}

// Highest extended function implemented
FEXCore::CPUID::FunctionResults CPUIDEmu::Function_8000_0000h(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};
  Res.eax = 0x8000001F;

  // EBX, EDX, ECX become the manufacturer id string
  // Just like cpuid function 0
#ifdef CPUID_AMD
  Res.ebx = CPUID_VENDOR_AMD1;
  Res.edx = CPUID_VENDOR_AMD2;
  Res.ecx = CPUID_VENDOR_AMD3;
#else
  Res.ebx = CPUID_VENDOR_INTEL1;
  Res.edx = CPUID_VENDOR_INTEL2;
  Res.ecx = CPUID_VENDOR_INTEL3;
#endif
  return Res;
}

// Extended processor and feature bits
FEXCore::CPUID::FunctionResults CPUIDEmu::Function_8000_0001h(uint32_t Leaf) const {

  // RDTSCP is disabled on WIN32/Wine because there is no sane way to query processor ID.
#ifndef _WIN32
  constexpr uint32_t SUPPORTS_RDTSCP = 1;
#else
  constexpr uint32_t SUPPORTS_RDTSCP = 0;
#endif
  FEXCore::CPUID::FunctionResults Res {};

  Res.eax = FAMILY_IDENTIFIER;

  Res.ecx = (1 << 0) |  // LAHF/SAHF
            (1 << 1) |  // 0 = Single core product, 1 = multi core product
            (0 << 2) |  // SVM
            (1 << 3) |  // Extended APIC register space
            (0 << 4) |  // LOCK MOV CR0 means MOV CR8
            (1 << 5) |  // ABM instructions
            (0 << 6) |  // SSE4a
            (0 << 7) |  // Misaligned SSE mode
            (1 << 8) |  // PREFETCHW
            (0 << 9) |  // OS visible workaround support
            (0 << 10) | // Instruction based sampling support
            (0 << 11) | // XOP
            (0 << 12) | // SKINIT
            (0 << 13) | // Watchdog timer support
            (0 << 14) | // Reserved
            (0 << 15) | // Lightweight profiling support
            (0 << 16) | // FMA4
            (1 << 17) | // Translation cache extension
            (0 << 18) | // Reserved
            (0 << 19) | // Reserved
            (0 << 20) | // Reserved
            (0 << 21) | // XOP-TBM
            (0 << 22) | // Topology extensions support
            (0 << 23) | // Core performance counter extensions
            (0 << 24) | // NB performance counter extensions
            (0 << 25) | // Reserved
            (0 << 26) | // Data breakpoints extensions
            (0 << 27) | // Performance TSC
            (0 << 28) | // L2 perf counter extensions
            (0 << 29) | // MONITORX
            (0 << 30) | // Reserved
            (0 << 31);  // Reserved

  Res.edx = (1 << 0) |                // FPU
            (1 << 1) |                // Virtual mode extensions
            (1 << 2) |                // Debugging extensions
            (1 << 3) |                // Page size extensions
            (1 << 4) |                // TSC
            (1 << 5) |                // MSR support
            (1 << 6) |                // PAE
            (1 << 7) |                // Machine Check Exception
            (1 << 8) |                // CMPXCHG8B
            (1 << 9) |                // APIC
            (0 << 10) |               // Reserved
            (1 << 11) |               // SYSCALL/SYSRET
            (1 << 12) |               // MTRR
            (1 << 13) |               // Page global extension
            (1 << 14) |               // Machine Check architecture
            (1 << 15) |               // CMOV
            (1 << 16) |               // Page attribute table
            (1 << 17) |               // Page-size extensions
            (0 << 18) |               // Reserved
            (0 << 19) |               // Reserved
            (1 << 20) |               // NX
            (0 << 21) |               // Reserved
            (1 << 22) |               // MMXExt
            (1 << 23) |               // MMX
            (1 << 24) |               // FXSAVE/FXRSTOR
            (1 << 25) |               // FXSAVE/FXRSTOR Optimizations
            (0 << 26) |               // 1 gigabit pages
            (SUPPORTS_RDTSCP << 27) | // RDTSCP
            (0 << 28) |               // Reserved
            (1 << 29) |               // Long Mode
            (1 << 30) |               // 3DNow! Extensions
            (1 << 31);                // 3DNow!
  return Res;
}

// Processor brand string
FEXCore::CPUID::FunctionResults CPUIDEmu::Function_8000_0002h(uint32_t Leaf) const {
  return Function_8000_0002h(Leaf, GetCPUID());
}

FEXCore::CPUID::FunctionResults CPUIDEmu::Function_8000_0003h(uint32_t Leaf) const {
  return Function_8000_0003h(Leaf, GetCPUID());
}

FEXCore::CPUID::FunctionResults CPUIDEmu::Function_8000_0004h(uint32_t Leaf) const {
  return Function_8000_0004h(Leaf, GetCPUID());
}

FEXCore::CPUID::FunctionResults CPUIDEmu::Function_8000_0002h(uint32_t Leaf, uint32_t CPU) const {
  FEXCore::CPUID::FunctionResults Res {};
  auto& Data = PerCPUData[CPU];
  memcpy(&Res, Data.ProductName, std::min(strlen(Data.ProductName), sizeof(FEXCore::CPUID::FunctionResults)));
  return Res;
}

FEXCore::CPUID::FunctionResults CPUIDEmu::Function_8000_0003h(uint32_t Leaf, uint32_t CPU) const {
  FEXCore::CPUID::FunctionResults Res {};
  auto& Data = PerCPUData[CPU];
  const auto RemainingStringSize = std::max<ssize_t>(0, strlen(Data.ProductName) - 16);
  memcpy(&Res, Data.ProductName + 16, std::min<size_t>(RemainingStringSize, sizeof(FEXCore::CPUID::FunctionResults)));
  return Res;
}

FEXCore::CPUID::FunctionResults CPUIDEmu::Function_8000_0004h(uint32_t Leaf, uint32_t CPU) const {
  FEXCore::CPUID::FunctionResults Res {};
  auto& Data = PerCPUData[CPU];
  const auto RemainingStringSize = std::max<ssize_t>(0, strlen(Data.ProductName) - 32);
  memcpy(&Res, Data.ProductName + 32, std::min<size_t>(RemainingStringSize, sizeof(FEXCore::CPUID::FunctionResults)));
  return Res;
}

// L1 Cache and TLB identifiers
FEXCore::CPUID::FunctionResults CPUIDEmu::Function_8000_0005h(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};

  // L1 TLB Information for 2MB and 4MB pages
  Res.eax = (64 << 0) |  // Number of TLB instruction entries
            (255 << 8) | // instruction TLB associativity type (full)
            (64 << 16) | // Number of TLB data entries
            (255 << 24); // data TLB associativity type (full)

  // L1 TLB Information for 4KB pages
  Res.ebx = (64 << 0) |  // Number of TLB instruction entries
            (255 << 8) | // instruction TLB associativity type (full)
            (64 << 16) | // Number of TLB data entries
            (255 << 24); // data TLB associativity type (full)

  // L1 data cache identifiers
  Res.ecx = (64 << 0) | // L1 data cache size line in bytes
            (1 << 8) |  // L1 data cachelines per tag
            (8 << 16) | // L1 data cache associativity
            (32 << 24); // L1 data cache size in KB

  // L1 instruction cache identifiers
  Res.edx = (64 << 0) | // L1 instruction cache line size in bytes
            (1 << 8) |  // L1 instruction cachelines per tag
            (4 << 16) | // L1 instruction cache associativity
            (64 << 24); // L1 instruction cache size in KB

  return Res;
}

// L2 Cache identifiers
FEXCore::CPUID::FunctionResults CPUIDEmu::Function_8000_0006h(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};

  // L2 TLB Information for 2MB and 4MB pages
  Res.eax = (1024 << 0) |  // Number of TLB instruction entries
            (6 << 12) |    // instruction TLB associativity type
            (1536 << 16) | // Number of TLB data entries
            (3 << 28);     // data TLB associativity type

  // L2 TLB Information for 4KB pages
  Res.ebx = (1024 << 0) |  // Number of TLB instruction entries
            (6 << 12) |    // instruction TLB associativity type
            (1536 << 16) | // Number of TLB data entries
            (5 << 28);     // data TLB associativity type

  // L2 cache identifiers
  Res.ecx = (64 << 0) |  // cacheline size
            (1 << 8) |   // cachelines per tag
            (6 << 12) |  // cache associativity
            (512 << 16); // L2 cache size in KB

  // L3 cache identifiers
  Res.edx = (64 << 0) | // cacheline size
            (1 << 8) |  // cachelines per tag
            (6 << 12) | // cache associativity
            (16 << 18); // L2 cache size in KB
  return Res;
}

// Advanced power management
FEXCore::CPUID::FunctionResults CPUIDEmu::Function_8000_0007h(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};
  Res.eax = (1 << 2); // APIC timer not affected by p-state
  Res.edx = (1 << 8); // Invariant TSC
  return Res;
}

// Virtual and physical address sizes
FEXCore::CPUID::FunctionResults CPUIDEmu::Function_8000_0008h(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};
  Res.eax = (48 << 0) | // PhysAddrSize = 48-bit
            (48 << 8) | // LinAddrSize = 48-bit
            (0 << 16);  // GuestPhysAddrSize == PhysAddrSize

  Res.ebx = (0 << 2) |                               // XSaveErPtr: Saving and restoring error pointers
            (0 << 1) |                               // IRPerf: Instructions retired count support
            (CTX->HostFeatures.SupportsCLZERO << 0); // CLZERO support

  uint32_t CoreCount = Cores - 1;
  Res.ecx = (0 << 16) |                                  // PerfTscSize: Performance timestamp count size
            ((uint32_t)std::log2(CoreCount + 1) << 12) | // ApicIdSize: Number of bits in ApicID
            (CoreCount << 0);                            // Count count subtract one

  return Res;
}

// TLB 1GB page identifiers
FEXCore::CPUID::FunctionResults CPUIDEmu::Function_8000_0019h(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};
  Res.eax = (0xF << 28) | // L1 DTLB associativity for 1GB pages
            (64 << 16) |  // L1 DTLB entry count for 1GB pages
            (0xF << 12) | // L1 ITLB associativity for 1GB pages
            (64 << 0);    // L1 ITLB entry count for 1GB pages

  Res.ebx = (0 << 28) | // L2 DTLB associativity for 1GB pages
            (0 << 16) | // L2 DTLB entry count for 1GB pages
            (0 << 12) | // L2 ITLB associativity for 1GB pages
            (0 << 0);   // L2 ITLB entry count for 1GB pages
  return Res;
}

// Deterministic cache parameters for each level
FEXCore::CPUID::FunctionResults CPUIDEmu::Function_8000_001Dh(uint32_t Leaf) const {
  // This is nearly a copy of CPUID function 4h
  // There are some minor changes though

  FEXCore::CPUID::FunctionResults Res {};
  constexpr uint32_t CacheType_Data = 1;
  constexpr uint32_t CacheType_Instruction = 2;
  constexpr uint32_t CacheType_Unified = 3;

  if (Leaf == 0) {
    // Report L1D
    Res.eax = CacheType_Data | // Cache type
              (0b001 << 5) |   // Cache level
              (1 << 8) |       // Self initializing cache level
              (0 << 9) |       // Fully associative
              (0 << 14);       // Maximum number of addressable IDs for logical processors sharing this cache (With SMT this would be 1)

    Res.ebx = (63 << 0) | // Line Size - 1 : Claiming 64 byte
              (0 << 12) | // Physical Line partitions
              (7 << 22);  // Associativity - 1 : Claiming 8 way

    // 32KB
    Res.ecx = 63; // Number of sets - 1 : Claiming 64 sets

    Res.edx = (0 << 0) | // Write-back invalidate
              (0 << 1);  // Cache inclusiveness - Includes lower caches
  } else if (Leaf == 1) {
    // Report L1I
    Res.eax = CacheType_Instruction | // Cache type
              (0b001 << 5) |          // Cache level
              (1 << 8) |              // Self initializing cache level
              (0 << 9) |              // Fully associative
              (0 << 14); // Maximum number of addressable IDs for logical processors sharing this cache (With SMT this would be 1)

    Res.ebx = (63 << 0) | // Line Size - 1 : Claiming 64 byte
              (0 << 12) | // Physical Line partitions
              (7 << 22);  // Associativity - 1 : Claiming 8 way

    // 32KB
    Res.ecx = 63; // Number of sets - 1 : Claiming 64 sets

    Res.edx = (0 << 0) | // Write-back invalidate
              (0 << 1);  // Cache inclusiveness - Includes lower caches
  } else if (Leaf == 2) {
    // Report L2
    Res.eax = CacheType_Unified | // Cache type
              (0b010 << 5) |      // Cache level
              (1 << 8) |          // Self initializing cache level
              (0 << 9) |          // Fully associative
              (0 << 14);          // Maximum number of addressable IDs for logical processors sharing this cache

    Res.ebx = (63 << 0) | // Line Size - 1 : Claiming 64 byte
              (0 << 12) | // Physical Line partitions
              (7 << 22);  // Associativity - 1 : Claiming 8 way

    // 512KB
    Res.ecx = 0x3FF; // Number of sets - 1 : Claiming 1024 sets

    Res.edx = (0 << 0) | // Write-back invalidate
              (0 << 1);  // Cache inclusiveness - Includes lower caches
  } else if (Leaf == 3) {
    // Report L3
    uint32_t CoreCount = Cores - 1;

    Res.eax = CacheType_Unified | // Cache type
              (0b011 << 5) |      // Cache level
              (1 << 8) |          // Self initializing cache level
              (0 << 9) |          // Fully associative
              (CoreCount << 14);  // Maximum number of addressable IDs for logical processors sharing this cache

    Res.ebx = (63 << 0) | // Line Size - 1 : Claiming 64 byte
              (0 << 12) | // Physical Line partitions
              (7 << 22);  // Associativity - 1 : Claiming 8 way

    // 8MB
    Res.ecx = 0x4000; // Number of sets - 1 : Claiming 16384 sets

    Res.edx = (0 << 0) | // Write-back invalidate
              (0 << 1);  // Cache inclusiveness - Includes lower caches
  }

  return Res;
}

FEXCore::CPUID::FunctionResults CPUIDEmu::Function_Reserved(uint32_t Leaf) const {
  FEXCore::CPUID::FunctionResults Res {};
  return Res;
}

FEXCore::CPUID::XCRResults CPUIDEmu::XCRFunction_0h() const {
  // This just returns XCR0
  FEXCore::CPUID::XCRResults Res {
    .eax = static_cast<uint32_t>(XCR0),
    .edx = static_cast<uint32_t>(XCR0 >> 32),
  };

  return Res;
}

CPUIDEmu::CPUIDEmu(const FEXCore::Context::ContextImpl* ctx)
  : CTX {ctx} {
  Cores = FEXCore::CPUInfo::CalculateNumberOfCPUs();

  // Setup some state tracking
  SetupHostHybridFlag();

  SetupFeatures();
}
} // namespace FEXCore
