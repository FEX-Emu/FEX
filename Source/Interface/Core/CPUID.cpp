#include "Interface/Core/CPUID.h"

namespace FEXCore {

CPUIDEmu::FunctionResults CPUIDEmu::Function_0h() {
  CPUIDEmu::FunctionResults Res{};

  Res.Res[0] = 0x16; // Let's say we are a Skylake
  // EBX, EDX, ECX become the manufacturer id string
  Res.Res[1] = 0x756E6547; // "Genu"
  Res.Res[2] = 0x49656E69; // "ineI"
  Res.Res[3] = 0x6C65746E; // "ntel"
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

  Res.Res[3]  &= ~(3 << 26); // Let's say that XSAVE isn't enabled by the OS. Prevents glibc from using XSAVE/XGETBV
  return Res;
}

// Cache and TLB description
CPUIDEmu::FunctionResults CPUIDEmu::Function_02h() {
  CPUIDEmu::FunctionResults Res{};
  return Res;
}

// Deterministic cache parameters for each level
CPUIDEmu::FunctionResults CPUIDEmu::Function_04h() {
  CPUIDEmu::FunctionResults Res{};
  return Res;
}

CPUIDEmu::FunctionResults CPUIDEmu::Function_07h() {
  CPUIDEmu::FunctionResults Res{};

  // Number of subfunctions
  Res.Res[0] = 0x0;
  Res.Res[1] =
    (1 << 0) | // FS/GS support
    (1 << 3) | // BMI 1 support
    (1 << 5) | // AVX2 support
    (1 << 7) | // SMEP support
    (1 << 8) // BMI2 support
    ;
  Res.Res[2] = ~0U;
  Res.Res[3] = ~0U;

  Res.Res[2] &= ~(1 << 20); // we don't support CET indirect branch tracking
  Res.Res[3] &= ~(
      (1 << 7)  | // we don't support CET shadow stack features
      (1 << 16) | // Remove AVX512F
      (1 << 17) | // Remove AVX512DQ
      (1 << 28) | // Remove AVX512CD
      (1 << 30)   // Remove AVX512BW
      );
  return Res;
}

CPUIDEmu::FunctionResults CPUIDEmu::Function_0Dh() {
  CPUIDEmu::FunctionResults Res{};
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
  return Res;
}

// Advanced power management
CPUIDEmu::FunctionResults CPUIDEmu::Function_8000_0006h() {
  CPUIDEmu::FunctionResults Res{};
  Res.Res[0] = (1 << 2); // APIC timer not affected by p-state
  return Res;
}

CPUIDEmu::FunctionResults CPUIDEmu::Function_8000_0007h() {
  CPUIDEmu::FunctionResults Res{};
  return Res;
}

// Virtual and physical address sizes
CPUIDEmu::FunctionResults CPUIDEmu::Function_8000_0008h() {
  CPUIDEmu::FunctionResults Res{};
  return Res;
}

void CPUIDEmu::Init() {
  RegisterFunction(0, std::bind(&CPUIDEmu::Function_0h, this));
  RegisterFunction(1, std::bind(&CPUIDEmu::Function_01h, this));
  RegisterFunction(2, std::bind(&CPUIDEmu::Function_02h, this));
  RegisterFunction(4, std::bind(&CPUIDEmu::Function_04h, this));
  RegisterFunction(7, std::bind(&CPUIDEmu::Function_07h, this));
  RegisterFunction(0xD, std::bind(&CPUIDEmu::Function_0Dh, this));

  RegisterFunction(0x8000'0000, std::bind(&CPUIDEmu::Function_8000_0000h, this));
  RegisterFunction(0x8000'0001, std::bind(&CPUIDEmu::Function_8000_0001h, this));
  RegisterFunction(0x8000'0006, std::bind(&CPUIDEmu::Function_8000_0006h, this));
  RegisterFunction(0x8000'0007, std::bind(&CPUIDEmu::Function_8000_0007h, this));
  RegisterFunction(0x8000'0008, std::bind(&CPUIDEmu::Function_8000_0008h, this));
}
}

