#include "Interface/Core/CPUID.h"

namespace FEXCore {
//#define CPUID_AMD

CPUIDEmu::FunctionResults CPUIDEmu::Function_0h() {
  CPUIDEmu::FunctionResults Res{};

  // EBX, EDX, ECX become the manufacturer id string
#ifdef CPUID_AMD
  Res.Res[0] = 0x0D; // Let's say we are a Zen+
  Res.Res[1] = CPUID_VENDOR_AMD1;
  Res.Res[2] = CPUID_VENDOR_AMD2;
  Res.Res[3] = CPUID_VENDOR_AMD3;
#else
  Res.Res[0] = 0x16; // Let's say we are a Skylake
  Res.Res[1] = CPUID_VENDOR_INTEL1;
  Res.Res[2] = CPUID_VENDOR_INTEL2;
  Res.Res[3] = CPUID_VENDOR_INTEL3;
#endif
  return Res;
}

// Processor Info and Features bits
CPUIDEmu::FunctionResults CPUIDEmu::Function_01h() {
  CPUIDEmu::FunctionResults Res{};

  Res.Res[0] = 0 | // Stepping
    (0 << 4) | // Model
    (0 << 8) | // Family ID
    (0 << 12) | // Processor type
    (0 << 16) | // Extended model ID
    (0 << 20); // Extended family ID
  Res.Res[1] = 0 | // Brand index
    (8 << 8) | // Cache line size in bytes
    (8 << 16) | // Number of addressable IDs for the logical cores in the physical CPU
    (0 << 24); // Local APIC ID
  Res.Res[2] = ~0U; // Let's say we support every feature for fun
  Res.Res[3] = ~0U; // Let's say we support every feature for fun

  Res.Res[3]  &= ~(
      (1 << 1) |  // Remove CLMUL
      (3 << 26) | // Let's say that XSAVE isn't enabled by the OS. Prevents glibc from using XSAVE/XGETBV
      (1 << 9)  | // Remove SSSE3
      (1 << 19) | // Remove SSE4.1
      (1 << 20) | // Remove SSE4.2
      (1 << 25) | // Remove AES
      (1 << 28) | // Remove AVX
      (1 << 30)   // Remove RDRAND
      );

  return Res;
}

CPUIDEmu::FunctionResults CPUIDEmu::Function_07h() {
  CPUIDEmu::FunctionResults Res{};

  // Number of subfunctions
  Res.Res[0] = 0x0;
  Res.Res[1] =
    (1 << 0) | // FS/GS support
    (1 << 5) | // AVX2 support
    (1 << 7)   // SMEP support
    ;
  Res.Res[2] = ~0U;
  Res.Res[3] = ~0U;

  Res.Res[2] &= ~(
    (1 << 2)  | // Remove AVX5124VNNIW
    (1 << 3)  | // Remove AVX5124FMAPS
    (1 << 8)  | // Remove AVX512VP2INTERSECT
    (1 << 20)   // we don't support CET indirect branch tracking
    );

  Res.Res[3] &= ~(
      (1 << 1)  | // Remove AVX512VBMI
      (1 << 6)  | // Remove AVX512VBMI2
      (1 << 7)  | // we don't support CET shadow stack features
      (1 << 11) | // Remove AVX512VNNI
      (1 << 12) | // Remove AVX512BITALG
      (1 << 14)   // Remove AVX512VPOPCNTDQ
      );
  return Res;
}

// Highest extended function implemented
CPUIDEmu::FunctionResults CPUIDEmu::Function_8000_0000h() {
  CPUIDEmu::FunctionResults Res{};
  Res.Res[0] = 0x8000001F;
  return Res;
}

// Extended processor and feature bits
CPUIDEmu::FunctionResults CPUIDEmu::Function_8000_0001h() {
  CPUIDEmu::FunctionResults Res{};
  Res.Res[2] = ~0U; // Let's say we support every feature for fun
  Res.Res[3] = ~0U; // Let's say we support every feature for fun

  Res.Res[3] &= ~(
    (1 << 6)  | // Remove SSE4a
    (1 << 11) | // Remove XOP
    (1 << 16)   // Remove FMA4
    );
  return Res;
}

// Advanced power management
CPUIDEmu::FunctionResults CPUIDEmu::Function_8000_0007h() {
  CPUIDEmu::FunctionResults Res{};
  Res.Res[0] = (1 << 2); // APIC timer not affected by p-state
  return Res;
}

CPUIDEmu::FunctionResults CPUIDEmu::Function_Reserved() {
  CPUIDEmu::FunctionResults Res{};
  return Res;
}

void CPUIDEmu::Init() {
  RegisterFunction(0, std::bind(&CPUIDEmu::Function_0h, this));
  RegisterFunction(1, std::bind(&CPUIDEmu::Function_01h, this));
  // Cache and TLB information
  RegisterFunction(2, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Serial Number(previously), now reserved
  RegisterFunction(3, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Deterministic cache parameters for each level
  RegisterFunction(4, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Monitor/mwait
  RegisterFunction(5, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Thermal and power management
  RegisterFunction(6, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Extended feature flags
  RegisterFunction(7, std::bind(&CPUIDEmu::Function_07h, this));
  // Reserved
  RegisterFunction(8, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Direct Cache Access information
  RegisterFunction(9, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Architectural performance monitoring
  RegisterFunction(0x0A, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Extended topology enumeration
  RegisterFunction(0x0B, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Reserved
  RegisterFunction(0x0C, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Processor extended state enumeration
  RegisterFunction(0x0D, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Reserved
  RegisterFunction(0x0E, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Intel RDT monitoring
  RegisterFunction(0x0F, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Intel RDT allocation enumeration
  RegisterFunction(0x10, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Reserved
  RegisterFunction(0x11, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Intel SGX capability enumeration
  RegisterFunction(0x12, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Reserved
  RegisterFunction(0x13, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Intel Processor trace
  RegisterFunction(0x14, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Timestamp counter information
  RegisterFunction(0x15, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Processor frequency information
  RegisterFunction(0x16, std::bind(&CPUIDEmu::Function_Reserved, this));
  // SoC vendor attribute enumeration
  RegisterFunction(0x17, std::bind(&CPUIDEmu::Function_Reserved, this));

  // Hypervisor vendor string
  RegisterFunction(0x4000'0000, std::bind(&CPUIDEmu::Function_Reserved, this));

  // Largest extended function number
  RegisterFunction(0x8000'0000, std::bind(&CPUIDEmu::Function_8000_0000h, this));
  // Processor vendor
  RegisterFunction(0x8000'0001, std::bind(&CPUIDEmu::Function_8000_0001h, this));
  // Processor brand string
  RegisterFunction(0x8000'0002, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Processor brand string continued
  RegisterFunction(0x8000'0003, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Processor brand string continued
  RegisterFunction(0x8000'0004, std::bind(&CPUIDEmu::Function_Reserved, this));
  // L1 Cache and TLB identifiers
  RegisterFunction(0x8000'0005, std::bind(&CPUIDEmu::Function_Reserved, this));
  // L2 Cache identifiers
  RegisterFunction(0x8000'0006, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Advanced power management information
  RegisterFunction(0x8000'0007, std::bind(&CPUIDEmu::Function_8000_0007h, this));
  // Virtual and physical address sizes
  RegisterFunction(0x8000'0008, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Reserved
  RegisterFunction(0x8000'0009, std::bind(&CPUIDEmu::Function_Reserved, this));
  // SVM Revision
  RegisterFunction(0x8000'000A, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Reserved
  RegisterFunction(0x8000'000B, std::bind(&CPUIDEmu::Function_Reserved, this));
  RegisterFunction(0x8000'000C, std::bind(&CPUIDEmu::Function_Reserved, this));
  RegisterFunction(0x8000'000D, std::bind(&CPUIDEmu::Function_Reserved, this));
  RegisterFunction(0x8000'000E, std::bind(&CPUIDEmu::Function_Reserved, this));
  RegisterFunction(0x8000'000F, std::bind(&CPUIDEmu::Function_Reserved, this));
  RegisterFunction(0x8000'0010, std::bind(&CPUIDEmu::Function_Reserved, this));
  RegisterFunction(0x8000'0011, std::bind(&CPUIDEmu::Function_Reserved, this));
  RegisterFunction(0x8000'0012, std::bind(&CPUIDEmu::Function_Reserved, this));
  RegisterFunction(0x8000'0013, std::bind(&CPUIDEmu::Function_Reserved, this));
  RegisterFunction(0x8000'0014, std::bind(&CPUIDEmu::Function_Reserved, this));
  RegisterFunction(0x8000'0015, std::bind(&CPUIDEmu::Function_Reserved, this));
  RegisterFunction(0x8000'0016, std::bind(&CPUIDEmu::Function_Reserved, this));
  RegisterFunction(0x8000'0017, std::bind(&CPUIDEmu::Function_Reserved, this));
  RegisterFunction(0x8000'0018, std::bind(&CPUIDEmu::Function_Reserved, this));
  // TLB 1GB page identifiers
  RegisterFunction(0x8000'0019, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Performance optimization identifiers
  RegisterFunction(0x8000'001A, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Instruction based sampling identifiers
  RegisterFunction(0x8000'001B, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Lightweight profiling capabilities
  RegisterFunction(0x8000'001C, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Cache properties
  RegisterFunction(0x8000'001D, std::bind(&CPUIDEmu::Function_Reserved, this));
  // Extended APIC ID
  RegisterFunction(0x8000'001E, std::bind(&CPUIDEmu::Function_Reserved, this));
}
}

