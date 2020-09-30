#include "Interface/Context/Context.h"
#include "Interface/Core/CPUID.h"
#include "git_version.h"

#include <cstring>

namespace FEXCore {
//#define CPUID_AMD

CPUIDEmu::FunctionResults CPUIDEmu::Function_0h() {
  CPUIDEmu::FunctionResults Res{};

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
CPUIDEmu::FunctionResults CPUIDEmu::Function_01h() {
  CPUIDEmu::FunctionResults Res{};

  Res.eax = 0 | // Stepping
    (0 << 4) | // Model
    (0 << 8) | // Family ID
    (0 << 12) | // Processor type
    (0 << 16) | // Extended model ID
    (0 << 20); // Extended family ID
  Res.ebx = 0 | // Brand index
    (8 << 8) | // Cache line size in bytes
    (8 << 16) | // Number of addressable IDs for the logical cores in the physical CPU
    (0 << 24); // Local APIC ID
  Res.ecx =
    (1 <<  0) | // SSE3
    (0 <<  1) | // PCLMULQDQ
    (1 <<  2) | // DS area supports 64bit layout
    (1 <<  3) | // MWait
    (1 <<  4) | // DS-CPL
    (0 <<  5) | // VMX
    (0 <<  6) | // SMX
    (0 <<  7) | // Intel SpeedStep
    (0 <<  8) | // Thermal Monitor 2
    (1 <<  9) | // SSSE3
    (0 << 10) | // L1 context ID
    (0 << 11) | // Silicon debug
    (0 << 12) | // FMA3
    (1 << 13) | // CMPXCHG16B
    (0 << 14) | // xTPR update control
    (0 << 15) | // Perfmon and debug capability
    (0 << 16) | // Reserved
    (0 << 17) | // Process-context identifiers
    (1 << 18) | // Prefetching from memory mapped device
    (0 << 19) | // SSE4.1
    (0 << 20) | // SSE4.2
    (0 << 21) | // X2APIC
    (1 << 22) | // MOVBE
    (1 << 23) | // POPCNT
    (0 << 24) | // APIC TSC-Deadline
    (CTX->HostFeatures.SupportsAES << 25) | // AES
    (0 << 26) | // XSAVE
    (0 << 27) | // OSXSAVE
    (0 << 28) | // AVX
    (0 << 29) | // RDRAND
    (0 << 30) | // RDRAND
    (0 << 31);  // Always returns zero

  Res.edx =
    (1 <<  0) | // FPU
    (1 <<  1) | // Virtual 8086 mode enhancements
    (0 <<  2) | // Debugging extensions
    (0 <<  3) | // Page size extension
    (1 <<  4) | // RDTSC supported
    (1 <<  5) | // MSR supported
    (1 <<  6) | // PAE
    (1 <<  7) | // Machine Check exception
    (1 <<  8) | // CMPXCHG8B
    (1 <<  9) | // APIC on-chip
    (0 << 10) | // Reserved
    (1 << 11) | // SYSENTER/SYSEXIT
    (1 << 12) | // Memory Type Range registers, MTRRs are supported
    (1 << 13) | // Page Global bit
    (1 << 14) | // Machine Check architecture
    (1 << 15) | // CMOV
    (1 << 16) | // Page Attribute Table
    (1 << 17) | // 36bit page size extension
    (0 << 18) | // Processor serial number
    (0 << 19) | // CLFLUSH
    (0 << 20) | // Reserved
    (0 << 21) | // Debug store
    (0 << 22) | // Thermal monitor and software controled clock
    (1 << 23) | // MMX
    (1 << 24) | // FXSAVE/FXRSTOR
    (1 << 25) | // SSE
    (1 << 26) | // SSE
    (1 << 27) | // Self Snoop
    (1 << 28) | // Max APIC IDs reserved field is valid
    (1 << 29) | // Thermal monitor
    (0 << 30) | // Reserved
    (1 << 31);  // Pending break enable
  return Res;
}

CPUIDEmu::FunctionResults CPUIDEmu::Function_06h() {
  CPUIDEmu::FunctionResults Res{};
  Res.eax = (1 << 2); // Always running APIC
  Res.ecx = (0 << 3); // Intel performance energy bias preference (EPB)
  return Res;
}

CPUIDEmu::FunctionResults CPUIDEmu::Function_07h() {
  CPUIDEmu::FunctionResults Res{};

  // Number of subfunctions
  Res.eax = 0x0;
  Res.ebx =
    (1 <<  0) | // FS/GS support
    (0 <<  1) | // TSC adjust MSR
    (0 <<  2) | // SGX
    (0 <<  3) | // BMI1
    (0 <<  4) | // Intel Hardware Lock Elison
    (0 <<  5) | // AVX2 support
    (1 <<  6) | // FPU data pointer updated only on exception
    (1 <<  7) | // SMEP support
    (0 <<  8) | // BMI2
    (0 <<  9) | // Enhanced REP MOVSB/STOSB
    (1 << 10) | // INVPCID for system software control of process-context
    (0 << 11) | // Restricted transactional memory
    (0 << 12) | // Intel resource directory technology Monitoring
    (1 << 13) | // Deprecates FPU CS and DS
    (0 << 14) | // Intel MPX
    (0 << 15) | // Intel Resource Directory Technology Allocation
    (0 << 16) | // Reserved
    (0 << 17) | // Reserved
    (0 << 18) | // RDSEED
    (0 << 19) | // ADCX and ADOX instructions
    (0 << 20) | // SMAP Supervisor mode access prevention and CLAC/STAC instructions
    (0 << 21) | // Reserved
    (0 << 22) | // Reserved
    (0 << 23) | // CLFLUSHOPT instruction
    (0 << 24) | // CLWB instruction
    (0 << 25) | // Intel processor trace
    (0 << 26) | // Reserved
    (0 << 27) | // Reserved
    (0 << 28) | // Reserved
    (0 << 29) | // SHA instructions
    (0 << 30) | // Reserved
    (0 << 31);  // Reserved

  Res.ecx =
    (1 <<  0) | // PREFETCHWT1
    (0 <<  1) | // AVX512VBMI
    (0 <<  2) | // Usermode instruction prevention
    (0 <<  3) | // Protection keys for user mode pages
    (1 <<  4) | // OS protection keys
    (0 <<  5) | // waitpkg
    (0 <<  6) | // AVX512_VBMI2
    (0 <<  7) | // CET shadow stack
    (0 <<  8) | // GFNI
    (0 <<  9) | // VAES
    (0 << 10) | // VPCLMULQDQ
    (0 << 11) | // AVX512_VNNI
    (0 << 12) | // AVX512_BITALG
    (0 << 13) | // Intel Total Memory Encryption
    (0 << 14) | // AVX512_VPOPCNTDQ
    (0 << 15) | // Reserved
    (0 << 16) | // 5 Level page tables
    (0 << 17) | // MPX MAWAU
    (0 << 18) | // MPX MAWAU
    (0 << 19) | // MPX MAWAU
    (0 << 20) | // MPX MAWAU
    (0 << 21) | // MPX MAWAU
    (0 << 22) | // RDPID Read Processor ID
    (0 << 23) | // Reserved
    (0 << 24) | // Reserved
    (0 << 25) | // CLDEMOTE
    (0 << 26) | // Reserved
    (0 << 27) | // MOVDIRI
    (0 << 28) | // MOVDIR64B
    (0 << 29) | // Reserved
    (0 << 30) | // SGX Launch configuration
    (0 << 31);  // Reserved

  Res.edx =
    (0 <<  0) | // Reserved
    (0 <<  1) | // Reserved
    (0 <<  2) | // AVX512_4VNNIW
    (0 <<  3) | // AVX512_4FMAPS
    (0 <<  4) | // Fast Short Rep Mov
    (0 <<  5) | // Reserved
    (0 <<  6) | // Reserved
    (0 <<  7) | // Reserved
    (0 <<  8) | // AVX512_VP2INTERSECT
    (0 <<  9) | // Reserved
    (0 << 10) | // VERW clears CPU buffers
    (0 << 11) | // Reserved
    (0 << 12) | // Reserved
    (0 << 13) | // Reserved
    (0 << 14) | // SERIALIZE instruction
    (0 << 15) | // Reserved
    (0 << 16) | // Reserved
    (0 << 17) | // Reserved
    (0 << 18) | // Intel PCONFIG
    (0 << 19) | // Intel Architectural LBR
    (0 << 20) | // Intel CET
    (0 << 21) | // Reserved
    (0 << 22) | // Reserved
    (0 << 23) | // Reserved
    (0 << 24) | // Reserved
    (0 << 25) | // Reserved
    (0 << 26) | // Reserved
    (0 << 27) | // Reserved
    (0 << 28) | // L1D Flush
    (0 << 29) | // Arch capabilities
    (0 << 30) | // Reserved
    (0 << 31);  // Reserved

  return Res;
}

// Highest extended function implemented
CPUIDEmu::FunctionResults CPUIDEmu::Function_8000_0000h() {
  CPUIDEmu::FunctionResults Res{};
  Res.eax = 0x8000001F;
  return Res;
}

// Extended processor and feature bits
CPUIDEmu::FunctionResults CPUIDEmu::Function_8000_0001h() {
  CPUIDEmu::FunctionResults Res{};

  Res.eax = 0 | // Stepping
    (0 << 4) | // Model
    (0 << 8) | // Family ID
    (0 << 12) | // Processor type
    (0 << 16) | // Extended model ID
    (0 << 20); // Extended family ID

  Res.ecx =
    (1 <<  0) | // LAHF/SAHF
    (1 <<  1) | // 0 = Single core product, 1 = multi core product
    (0 <<  2) | // SVM
    (1 <<  3) | // Extended APIC register space
    (0 <<  4) | // LOCK MOV CR0 means MOV CR8
    (1 <<  5) | // ABM instructions
    (0 <<  6) | // SSE4a
    (0 <<  7) | // Misaligned SSE mode
    (1 <<  8) | // PREFETCHW
    (0 <<  9) | // OS visible workaround support
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
    (0 << 21) | // Reserved
    (1 << 22) | // Topology extensions support
    (1 << 23) | // Core performance counter extensions
    (1 << 24) | // NB performance counter extensions
    (0 << 25) | // Reserved
    (0 << 26) | // Data breakpoints extensions
    (1 << 27) | // Performance TSC
    (1 << 28) | // L2 perf counter extensions
    (0 << 29) | // Reserved
    (0 << 30) | // Reserved
    (0 << 31);  // Reserved

  Res.edx =
    (1 <<  0) | // FPU
    (1 <<  1) | // Virtual mode extensions
    (1 <<  2) | // Debugging extensions
    (1 <<  3) | // Page size extensions
    (1 <<  4) | // TSC
    (1 <<  5) | // MSR support
    (1 <<  6) | // PAE
    (1 <<  7) | // Machine Check Exception
    (1 <<  8) | // CMPXCHG8B
    (1 <<  9) | // APIC
    (0 << 10) | // Reserved
    (1 << 11) | // SYSCALL/SYSRET
    (1 << 12) | // MTRR
    (1 << 13) | // Page global extension
    (1 << 14) | // Machine Check architecture
    (1 << 15) | // CMOV
    (1 << 16) | // Page attribute table
    (1 << 17) | // Page-size extensions
    (0 << 18) | // Reserved
    (0 << 19) | // Reserved
    (1 << 20) | // NX
    (0 << 21) | // Reserved
    (1 << 22) | // MMXExt
    (1 << 23) | // MMX
    (1 << 24) | // FXSAVE/FXRSTOR
    (1 << 25) | // FXSAVE/FXRSTOR Optimizations
    (1 << 26) | // 1 gigabit pages
    (0 << 27) | // RDTSCP
    (0 << 28) | // Reserved
    (1 << 29) | // Long Mode
    (0 << 30) | // 3DNow! Extensions
    (0 << 31);  // 3DNow!
  return Res;
}

constexpr char ProcessorBrand[48] = {
  "FEX-"
  GIT_SHORT_HASH
  "\0"
};

//Processor brand string
CPUIDEmu::FunctionResults CPUIDEmu::Function_8000_0002h() {
  CPUIDEmu::FunctionResults Res{};
  memcpy(&Res, &ProcessorBrand[0], sizeof(CPUIDEmu::FunctionResults));
  return Res;
}

CPUIDEmu::FunctionResults CPUIDEmu::Function_8000_0003h() {
  CPUIDEmu::FunctionResults Res{};
  memcpy(&Res, &ProcessorBrand[16], sizeof(CPUIDEmu::FunctionResults));
  return Res;
}

CPUIDEmu::FunctionResults CPUIDEmu::Function_8000_0004h() {
  CPUIDEmu::FunctionResults Res{};
  memcpy(&Res, &ProcessorBrand[32], sizeof(CPUIDEmu::FunctionResults));
  return Res;
}

// Advanced power management
CPUIDEmu::FunctionResults CPUIDEmu::Function_8000_0007h() {
  CPUIDEmu::FunctionResults Res{};
  Res.eax = (1 << 2); // APIC timer not affected by p-state
  return Res;
}

CPUIDEmu::FunctionResults CPUIDEmu::Function_Reserved() {
  CPUIDEmu::FunctionResults Res{};
  return Res;
}

void CPUIDEmu::Init(FEXCore::Context::Context *ctx) {
  CTX = ctx;
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
  RegisterFunction(6, std::bind(&CPUIDEmu::Function_06h, this));
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
  RegisterFunction(0x8000'0002, std::bind(&CPUIDEmu::Function_8000_0002h, this));
  // Processor brand string continued
  RegisterFunction(0x8000'0003, std::bind(&CPUIDEmu::Function_8000_0003h, this));
  // Processor brand string continued
  RegisterFunction(0x8000'0004, std::bind(&CPUIDEmu::Function_8000_0004h, this));
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
  // AMD Secure Encryption
  RegisterFunction(0x8000'001F, std::bind(&CPUIDEmu::Function_Reserved, this));

}
}

