// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Core/CPUID.h>
#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/vector.h>

#include <cstdint>
#include <unordered_map>
#include <utility>

namespace FEXCore {
namespace Context {
  class ContextImpl;
}

uint32_t GetCycleCounterFrequency();

// Debugging define to switch what family of CPU we execute as.
// Might be useful if an application makes an assumption about a CPU.
// #define CPUID_AMD
class CPUIDEmu final {
private:
  constexpr static uint32_t CPUID_VENDOR_INTEL1 = 0x756E6547; // "Genu"
  constexpr static uint32_t CPUID_VENDOR_INTEL2 = 0x49656E69; // "ineI"
  constexpr static uint32_t CPUID_VENDOR_INTEL3 = 0x6C65746E; // "ntel"

  constexpr static uint32_t CPUID_VENDOR_AMD1 = 0x68747541; // "Auth"
  constexpr static uint32_t CPUID_VENDOR_AMD2 = 0x69746E65; // "enti"
  constexpr static uint32_t CPUID_VENDOR_AMD3 = 0x444D4163; // "cAMD"

public:
  CPUIDEmu(const FEXCore::Context::ContextImpl* ctx);

  // X86 cacheline size effectively has to be hardcoded to 64
  // if we report anything differently then applications are likely to break
  constexpr static uint64_t CACHELINE_SIZE = 64;

  FEXCore::CPUID::FunctionResults RunFunction(uint32_t Function, uint32_t Leaf) const {
    if (Function < Primary.size()) {
      const auto Handler = Primary[Function];
      return (this->*Handler)(Leaf);
    }

    constexpr uint32_t HypervisorBase = 0x4000'0000;
    if (Function >= HypervisorBase && Function < (HypervisorBase + Hypervisor.size())) {
      const auto Handler = Hypervisor[Function - HypervisorBase];
      return (this->*Handler)(Leaf);
    }

    constexpr uint32_t ExtendedBase = 0x8000'0000;
    if (Function >= ExtendedBase && Function < (ExtendedBase + Extended.size())) {
      const auto Handler = Extended[Function - ExtendedBase];
      return (this->*Handler)(Leaf);
    }

    return Function_Reserved(Leaf);
  }

  FEXCore::CPUID::FunctionResults RunFunctionName(uint32_t Function, uint32_t Leaf, uint32_t CPU) const {
    if (Function == 0x8000'0002U) {
      return Function_8000_0002h(Leaf, CPU % PerCPUData.size());
    } else if (Function == 0x8000'0003U) {
      return Function_8000_0003h(Leaf, CPU % PerCPUData.size());
    } else {
      return Function_8000_0004h(Leaf, CPU % PerCPUData.size());
    }
  }

  FEXCore::CPUID::XCRResults RunXCRFunction(uint32_t Function) const {
    if (Function >= 1) {
      // XCR function 1 is not yet supported.
      return {};
    }

    return XCRFunction_0h();
  }

  bool DoesXCRFunctionReportConstantData(uint32_t Function) const {
    // Every function currently returns constant data.
    return true;
  }

  enum class SupportsConstant {
    CONSTANT,
    NONCONSTANT,
  };
  enum class NeedsLeafConstant {
    NEEDSLEAFCONSTANT,
    NOLEAFCONSTANT,
  };
  struct FunctionConstant {
    SupportsConstant SupportsConstantFunction;
    NeedsLeafConstant NeedsLeaf;
  };

  static constexpr FunctionConstant DoesFunctionReportConstantData(uint32_t Function) {
    if (Function < Primary.size()) {
      return Primary_Constant[Function];
    }

    constexpr uint32_t HypervisorBase = 0x4000'0000;
    if (Function >= HypervisorBase && Function < (HypervisorBase + Hypervisor.size())) {
      return Hypervisor_Constant[Function - HypervisorBase];
    }

    constexpr uint32_t ExtendedBase = 0x8000'0000;
    if (Function >= ExtendedBase && Function < (ExtendedBase + Extended.size())) {
      return Extended_Constant[Function - ExtendedBase];
    }

    // Anything unsupported is known constant return of reserved data.
    return {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT};
  }

private:
  const FEXCore::Context::ContextImpl* CTX;
  bool SupportsCPUIndexInTPIDRRO {};
  bool Hybrid {};
  uint32_t Cores {};
  FEX_CONFIG_OPT(HideHypervisorBit, HIDEHYPERVISORBIT);

  // XFEATURE_ENABLED_MASK
  // Mask that configures what features are enabled on the CPU.
  // Affects XSAVE and XRSTOR when modified.
  // Bit layout is as follows.
  // [0]     - x87 enabled
  // [1]     - SSE enabled
  // [2]     - YMM enabled (256-bit SSE)
  // [8:3]   - Reserved. MBZ.
  // [9]     - MPK
  // [10]    - Reserved. MBZ.
  // [11]    - CET_U
  // [12]    - CET_S
  // [61:13] - Reserved. MBZ.
  // [62]    - LWP (Lightweight profiling)
  // [63]    - Reserved for XCR bit vector expansion. MBZ.
  // Always enable x87 and SSE by default.
  constexpr static uint64_t XCR0_X87 = 1ULL << 0;
  constexpr static uint64_t XCR0_SSE = 1ULL << 1;
  constexpr static uint64_t XCR0_AVX = 1ULL << 2;

  struct FeaturesConfig {
    uint64_t SHA  : 1;
    uint64_t _pad : 63;
  };

  FeaturesConfig Features {
    .SHA = 1,
  };

  uint64_t XCR0 {XCR0_X87 | XCR0_SSE};

  uint32_t SupportsAVX() const {
    return (XCR0 & XCR0_AVX) ? 1 : 0;
  }

  using FunctionHandler = FEXCore::CPUID::FunctionResults (CPUIDEmu::*)(uint32_t Leaf) const;

  struct CPUData {
    const char* ProductName {};
#ifdef _M_ARM_64
    uint32_t MIDR {};
#endif
    bool IsBig {};
  };
  fextl::vector<CPUData> PerCPUData {};

  // Functions
  FEXCore::CPUID::FunctionResults Function_0h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_01h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_02h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_04h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_06h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_07h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_0Dh(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_15h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_1Ah(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_4000_0000h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_4000_0001h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_8000_0000h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_8000_0001h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_8000_0002h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_8000_0003h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_8000_0004h(uint32_t Leaf) const;

  FEXCore::CPUID::FunctionResults Function_8000_0002h(uint32_t Leaf, uint32_t CPU) const;
  FEXCore::CPUID::FunctionResults Function_8000_0003h(uint32_t Leaf, uint32_t CPU) const;
  FEXCore::CPUID::FunctionResults Function_8000_0004h(uint32_t Leaf, uint32_t CPU) const;

  FEXCore::CPUID::FunctionResults Function_8000_0005h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_8000_0006h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_8000_0007h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_8000_0008h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_8000_0019h(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_8000_001Dh(uint32_t Leaf) const;
  FEXCore::CPUID::FunctionResults Function_Reserved(uint32_t Leaf) const;

  FEXCore::CPUID::XCRResults XCRFunction_0h() const;

  void SetupHostHybridFlag();
  void SetupFeatures();
  static constexpr size_t PRIMARY_FUNCTION_COUNT = 27;
  static constexpr size_t HYPERVISOR_FUNCTION_COUNT = 2;
  static constexpr size_t EXTENDED_FUNCTION_COUNT = 32;
  static constexpr std::array<FunctionHandler, PRIMARY_FUNCTION_COUNT> Primary = {
    // 0: Highest function parameter and ID
    &CPUIDEmu::Function_0h,
    // 1: Processor info
    &CPUIDEmu::Function_01h,
    // 2: Cache and TLB info
    &CPUIDEmu::Function_02h,
    // 3: Serial Number(previously), now reserved
    &CPUIDEmu::Function_Reserved,
#ifndef CPUID_AMD
    // 4: Deterministic cache parameters for each level
    &CPUIDEmu::Function_04h,
#else
    &CPUIDEmu::Function_Reserved,
#endif
    // 5: Monitor/mwait
    &CPUIDEmu::Function_Reserved,
    // 6: Thermal and power management
    &CPUIDEmu::Function_06h,
    // 7: Extended feature flags
    &CPUIDEmu::Function_07h,
    // 0x08: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 9: Direct Cache Access information
    &CPUIDEmu::Function_Reserved,
    // 0x0A: Architectural performance monitoring
    &CPUIDEmu::Function_Reserved,
    // 0x0B: Extended topology enumeration
    &CPUIDEmu::Function_Reserved,
    // 0x0C: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x0D: Processor extended state enumeration
    &CPUIDEmu::Function_0Dh,
    // 0x0E: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x0F: Intel RDT monitoring
    &CPUIDEmu::Function_Reserved,
    // 0x10: Intel RDT allocation enumeration
    &CPUIDEmu::Function_Reserved,
    // 0x12: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x12: Intel SGX capability enumeration
    &CPUIDEmu::Function_Reserved,
    // 0x13: Reserved
    &CPUIDEmu::Function_Reserved,
    // 0x14: Intel Processor trace
    &CPUIDEmu::Function_Reserved,
#ifndef CPUID_AMD
    // Timestamp counter information
    // Doesn't exist on AMD hardware
    &CPUIDEmu::Function_15h,
#else
    &CPUIDEmu::Function_Reserved,
#endif
    // 0x16: Processor frequency information
    &CPUIDEmu::Function_Reserved,
    // 0x17: SoC vendor attribute enumeration
    &CPUIDEmu::Function_Reserved,
    // 0x18: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x19: Reserved?
    &CPUIDEmu::Function_Reserved,
#ifndef CPUID_AMD
    // 0x1A: Hybrid Information Sub-leaf
    &CPUIDEmu::Function_1Ah,
#else
    &CPUIDEmu::Function_Reserved,
#endif
  };

  static constexpr std::array<FunctionConstant, PRIMARY_FUNCTION_COUNT> Primary_Constant = {{
    // 0: Highest function parameter and ID
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 1: Processor info
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 2: Cache and TLB info
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 3: Serial Number(previously), now reserved
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
#ifndef CPUID_AMD
    // 4: Deterministic cache parameters for each level
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NEEDSLEAFCONSTANT},
#else
    // 4: Reserved
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
#endif
    // 5: Monitor/mwait
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 6: Thermal and power management
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 7: Extended feature flags
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NEEDSLEAFCONSTANT},
    // 0x08: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 9: Direct Cache Access information
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x0A: Architectural performance monitoring
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x0B: Extended topology enumeration
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x0C: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x0D: Processor extended state enumeration
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NEEDSLEAFCONSTANT},
    // 0x0E: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x0F: Intel RDT monitoring
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x10: Intel RDT allocation enumeration
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x12: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x12: Intel SGX capability enumeration
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x13: Reserved
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x14: Intel Processor trace
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
#ifndef CPUID_AMD
    // 0x15: Timestamp counter information
    // Doesn't exist on AMD hardware
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
#else
    // 0x15: Reserved
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
#endif
    // 0x16: Processor frequency information
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x17: SoC vendor attribute enumeration
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x18: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x19: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
#ifndef CPUID_AMD
    // 0x1A: Hybrid Information Sub-leaf
    {SupportsConstant::NONCONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
#else
    // 0x1A: Reserved
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
#endif
  }};

  static constexpr std::array<FunctionHandler, HYPERVISOR_FUNCTION_COUNT> Hypervisor = {
    // Hypervisor CPUID information leaf
    &CPUIDEmu::Function_4000_0000h,
    // FEX-Emu specific leaf
    &CPUIDEmu::Function_4000_0001h,
  };

  static constexpr std::array<FunctionConstant, HYPERVISOR_FUNCTION_COUNT> Hypervisor_Constant = {{
    // Hypervisor CPUID information leaf
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // FEX-Emu specific leaf
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NEEDSLEAFCONSTANT},
  }};

  static constexpr std::array<FunctionHandler, EXTENDED_FUNCTION_COUNT> Extended = {
    // Largest extended function number
    &CPUIDEmu::Function_8000_0000h,
    // Processor vendor
    &CPUIDEmu::Function_8000_0001h,
    // Processor brand string
    &CPUIDEmu::Function_8000_0002h,
    // Processor brand string continued
    &CPUIDEmu::Function_8000_0003h,
    // Processor brand string continued
    &CPUIDEmu::Function_8000_0004h,
#ifdef CPUID_AMD
    // 0x8000'0005: L1 Cache and TLB identifiers
    &CPUIDEmu::Function_8000_0005h,
#else
    &CPUIDEmu::Function_Reserved,
#endif
    // 0x8000'0006: L2 Cache identifiers
    &CPUIDEmu::Function_8000_0006h,
    // 0x8000'0007: Advanced power management information
    &CPUIDEmu::Function_8000_0007h,
    // 0x8000'0008: Virtual and physical address sizes
    &CPUIDEmu::Function_8000_0008h,
    // 0x8000'0009: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x8000'000A: SVM Revision
    &CPUIDEmu::Function_Reserved,
    // 0x8000'000B: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x8000'000C: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x8000'000D: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x8000'000E: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x8000'000F: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x8000'0010: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x8000'0011: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x8000'0012: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x8000'0013: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x8000'0014: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x8000'0015: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x8000'0016: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x8000'0017: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x8000'0018: Reserved?
    &CPUIDEmu::Function_Reserved,
    // 0x8000'0019: TLB 1GB page identifiers
    &CPUIDEmu::Function_8000_0019h,
    // 0x8000'001A: Performance optimization identifiers
    &CPUIDEmu::Function_Reserved,
    // 0x8000'001B: Instruction based sampling identifiers
    &CPUIDEmu::Function_Reserved,
    // 0x8000'001C: Lightweight profiling capabilities
    &CPUIDEmu::Function_Reserved,
#ifdef CPUID_AMD
    // 0x8000'001D: Cache properties
    &CPUIDEmu::Function_8000_001Dh,
#else
    &CPUIDEmu::Function_Reserved,
#endif
    // 0x8000'001E: Extended APIC ID
    &CPUIDEmu::Function_Reserved,
    // 0x8000'001F: AMD Secure Encryption
    &CPUIDEmu::Function_Reserved,
  };

  static constexpr std::array<FunctionConstant, EXTENDED_FUNCTION_COUNT> Extended_Constant = {{
    // Largest extended function number
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // Processor vendor
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // Processor brand string
    {SupportsConstant::NONCONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // Processor brand string continued
    {SupportsConstant::NONCONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // Processor brand string continued
    {SupportsConstant::NONCONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
#ifdef CPUID_AMD
    // 0x8000'0005: L1 Cache and TLB identifiers
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
#else
    // 0x8000'0005: Reserved
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
#endif
    // 0x8000'0006: L2 Cache identifiers
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'0007: Advanced power management information
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'0008: Virtual and physical address sizes
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'0009: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'000A: SVM Revision
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'000B: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'000C: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'000D: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'000E: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'000F: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'0010: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'0011: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'0012: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'0013: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'0014: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'0015: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'0016: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'0017: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'0018: Reserved?
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'0019: TLB 1GB page identifiers
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'001A: Performance optimization identifiers
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'001B: Instruction based sampling identifiers
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'001C: Lightweight profiling capabilities
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
#ifdef CPUID_AMD
    // 0x8000'001D: Cache properties
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NEEDSLEAFCONSTANT},
#else
    // 0x8000'001D: Reserved
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
#endif
    // 0x8000'001E: Extended APIC ID
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
    // 0x8000'001F: AMD Secure Encryption
    {SupportsConstant::CONSTANT, NeedsLeafConstant::NOLEAFCONSTANT},
  }};

  using GetCPUIDPtr = uint32_t (*)();
  GetCPUIDPtr GetCPUID;
};
} // namespace FEXCore
