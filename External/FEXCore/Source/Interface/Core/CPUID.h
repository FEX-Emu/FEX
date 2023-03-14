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
  // X86 cacheline size effectively has to be hardcoded to 64
  // if we report anything differently then applications are likely to break
  constexpr static uint64_t CACHELINE_SIZE = 64;

  void Init(FEXCore::Context::ContextImpl *ctx);

  FEXCore::CPUID::FunctionResults RunFunction(uint32_t Function, uint32_t Leaf) {
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

  FEXCore::CPUID::FunctionResults RunFunctionName(uint32_t Function, uint32_t Leaf, uint32_t CPU) {
    if (Function == 0x8000'0002U)
      return Function_8000_0002h(Leaf, CPU % PerCPUData.size());
    else if (Function == 0x8000'0003U)
      return Function_8000_0003h(Leaf, CPU % PerCPUData.size());
    else
      return Function_8000_0004h(Leaf, CPU % PerCPUData.size());
  }

private:
  FEXCore::Context::ContextImpl *CTX;
  bool Hybrid{};
  FEX_CONFIG_OPT(Cores, THREADS);
  FEX_CONFIG_OPT(HideHypervisorBit, HIDEHYPERVISORBIT);

  using FunctionHandler = FEXCore::CPUID::FunctionResults (CPUIDEmu::*)(uint32_t Leaf);
  struct CPUData {
    const char *ProductName{};
#ifdef _M_ARM_64
    uint32_t MIDR{};
#endif
    bool IsBig{};
  };
  fextl::vector<CPUData> PerCPUData{};

  // Functions
  FEXCore::CPUID::FunctionResults Function_0h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_01h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_02h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_04h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_06h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_07h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_0Dh(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_15h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_1Ah(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_4000_0000h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_4000_0001h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0000h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0001h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0002h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0003h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0004h(uint32_t Leaf);

  FEXCore::CPUID::FunctionResults Function_8000_0002h(uint32_t Leaf, uint32_t CPU);
  FEXCore::CPUID::FunctionResults Function_8000_0003h(uint32_t Leaf, uint32_t CPU);
  FEXCore::CPUID::FunctionResults Function_8000_0004h(uint32_t Leaf, uint32_t CPU);

  FEXCore::CPUID::FunctionResults Function_8000_0005h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0006h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0007h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0008h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0019h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_001Dh(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_Reserved(uint32_t Leaf);

  void SetupHostHybridFlag();
  static constexpr std::array<FunctionHandler, 27> Primary = {
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

  static constexpr std::array<FunctionHandler, 2> Hypervisor = {
    // Hypervisor CPUID information leaf
    &CPUIDEmu::Function_4000_0000h,
    // FEX-Emu specific leaf
    &CPUIDEmu::Function_4000_0001h,
  };

  static constexpr std::array<FunctionHandler, 32> Extended = {
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
};
}
