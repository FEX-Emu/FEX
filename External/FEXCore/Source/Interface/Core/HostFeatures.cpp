#include "Interface/Core/CPUID.h"
#include <FEXCore/Core/HostFeatures.h>

#if defined(_M_ARM_64) || defined(VIXL_SIMULATOR)
#include "aarch64/assembler-aarch64.h"
#include "aarch64/cpu-aarch64.h"
#include "aarch64/disasm-aarch64.h"
#include "aarch64/assembler-aarch64.h"
#endif

#ifdef _M_X86_64
#include "Interface/Core/Dispatcher/X86Dispatcher.h"
#endif

namespace FEXCore {

// Data Zero Prohibited flag
// 0b0 = ZVA/GVA/GZVA permitted
// 0b1 = ZVA/GVA/GZVA prohibited
[[maybe_unused]] constexpr uint32_t DCZID_DZP_MASK = 0b1'0000;
// Log2 of the blocksize in 32-bit words
[[maybe_unused]] constexpr uint32_t DCZID_BS_MASK = 0b0'1111;

#ifdef _M_ARM_64
[[maybe_unused]] static uint32_t GetDCZID() {
  uint64_t Result{};
  __asm("mrs %[Res], DCZID_EL0"
      : [Res] "=r" (Result));
  return Result;
}

static uint32_t GetFPCR() {
  uint64_t Result{};
  __asm ("mrs %[Res], FPCR"
    : [Res] "=r" (Result));
  return Result;
}

static void SetFPCR(uint64_t Value) {
  __asm ("msr FPCR, %[Value]"
    :: [Value] "r" (Value));
}

#else
static uint32_t GetDCZID() {
  // Return unsupported
  return DCZID_DZP_MASK;
}
#endif

static void OverrideFeatures(HostFeatures *Features) {
  // Override features if the user has specifically called for it.
  FEX_CONFIG_OPT(HostFeatures, HOSTFEATURES);
  if (!HostFeatures()) {
    // Early exit if no features are overriden.
    return;
  }

  const bool DisableAVX = HostFeatures() & FEXCore::Config::HostFeatures::DISABLEAVX;
  const bool EnableAVX = HostFeatures() & FEXCore::Config::HostFeatures::ENABLEAVX;
  LogMan::Throw::AFmt(!(DisableAVX && EnableAVX), "Disabling and Enabling CPU features are mutually exclusive");

  const bool DisableSVE = HostFeatures() & FEXCore::Config::HostFeatures::DISABLESVE;
  const bool EnableSVE = HostFeatures() & FEXCore::Config::HostFeatures::ENABLESVE;
  LogMan::Throw::AFmt(!(DisableSVE && EnableSVE), "Disabling and Enabling CPU features are mutually exclusive");

  const bool DisableAFP = HostFeatures() & FEXCore::Config::HostFeatures::DISABLEAFP;
  const bool EnableAFP = HostFeatures() & FEXCore::Config::HostFeatures::ENABLEAFP;
  LogMan::Throw::AFmt(!(DisableAFP && EnableAFP), "Disabling and Enabling CPU features are mutually exclusive");

  const bool DisableLRCPC = HostFeatures() & FEXCore::Config::HostFeatures::DISABLELRCPC;
  const bool EnableLRCPC = HostFeatures() & FEXCore::Config::HostFeatures::ENABLELRCPC;
  LogMan::Throw::AFmt(!(DisableLRCPC && EnableLRCPC), "Disabling and Enabling CPU features are mutually exclusive");

  const bool DisableLRCPC2 = HostFeatures() & FEXCore::Config::HostFeatures::DISABLELRCPC2;
  const bool EnableLRCPC2 = HostFeatures() & FEXCore::Config::HostFeatures::ENABLELRCPC2;
  LogMan::Throw::AFmt(!(DisableLRCPC2 && EnableLRCPC2), "Disabling and Enabling CPU features are mutually exclusive");

  const bool DisableCSSC = HostFeatures() & FEXCore::Config::HostFeatures::DISABLECSSC;
  const bool EnableCSSC = HostFeatures() & FEXCore::Config::HostFeatures::ENABLECSSC;
  LogMan::Throw::AFmt(!(DisableCSSC && EnableCSSC), "Disabling and Enabling CPU features are mutually exclusive");

  const bool DisablePMULL128 = HostFeatures() & FEXCore::Config::HostFeatures::DISABLEPMULL128;
  const bool EnablePMULL128 = HostFeatures() & FEXCore::Config::HostFeatures::ENABLEPMULL128;
  LogMan::Throw::AFmt(!(DisablePMULL128 && EnablePMULL128), "Disabling and Enabling CPU features are mutually exclusive");

  const bool DisableRNG = HostFeatures() & FEXCore::Config::HostFeatures::DISABLERNG;
  const bool EnableRNG = HostFeatures() & FEXCore::Config::HostFeatures::ENABLERNG;
  LogMan::Throw::AFmt(!(DisableRNG && EnableRNG), "Disabling and Enabling CPU features are mutually exclusive");

  if (EnableAVX) {
    Features->SupportsAVX = true;
  }
  else if (DisableAVX) {
    Features->SupportsAVX = false;
  }
  if (EnableSVE) {
    Features->SupportsSVE = true;
  }
  else if (DisableSVE) {
    Features->SupportsSVE = false;
  }
  if (EnableAFP) {
    Features->SupportsFlushInputsToZero = true;
  }
  else if (DisableAFP) {
    Features->SupportsFlushInputsToZero = false;
  }
  if (EnableLRCPC) {
    Features->SupportsRCPC = true;
  }
  else if (DisableLRCPC) {
    Features->SupportsRCPC = false;
  }
  if (EnableLRCPC2) {
    Features->SupportsTSOImm9 = true;
  }
  else if (DisableLRCPC2) {
    Features->SupportsTSOImm9 = false;
  }
  if (EnableCSSC) {
    Features->SupportsCSSC = true;
  }
  else if (DisableCSSC) {
    Features->SupportsCSSC = false;
  }
  if (EnablePMULL128) {
    Features->SupportsPMULL_128Bit = true;
  }
  else if (DisablePMULL128) {
    Features->SupportsPMULL_128Bit = false;
  }
  if (EnableRNG) {
    Features->SupportsRAND = true;
  }
  else if (DisableRNG) {
    Features->SupportsRAND = false;
  }
}

HostFeatures::HostFeatures() {
#if defined(_M_ARM_64) || defined(VIXL_SIMULATOR)
#ifdef VIXL_SIMULATOR
  auto Features = vixl::CPUFeatures::All();
#else
#ifndef _WIN32
  auto Features = vixl::CPUFeatures::InferFromOS();
#else
  // Need to use ID registers in WINE.
  auto Features = vixl::CPUFeatures::InferFromIDRegisters();
#endif
#endif
  SupportsAES = Features.Has(vixl::CPUFeatures::Feature::kAES);
  SupportsCRC = Features.Has(vixl::CPUFeatures::Feature::kCRC32);
  SupportsAtomics = Features.Has(vixl::CPUFeatures::Feature::kAtomics);
  SupportsRAND = Features.Has(vixl::CPUFeatures::Feature::kRNG);

  // Only supported when FEAT_AFP is supported
  SupportsFlushInputsToZero = Features.Has(vixl::CPUFeatures::Feature::kAFP);
  SupportsRCPC = Features.Has(vixl::CPUFeatures::Feature::kRCpc);
  SupportsTSOImm9 = Features.Has(vixl::CPUFeatures::Feature::kRCpcImm);
  SupportsPMULL_128Bit = Features.Has(vixl::CPUFeatures::Feature::kPmull1Q);
  SupportsCSSC = Features.Has(vixl::CPUFeatures::Feature::kCSSC);

  Supports3DNow = true;
  SupportsSSE4A = true;
#ifdef VIXL_SIMULATOR
  // Hardcode enable SVE with 256-bit wide registers.
  SupportsSVE = true;
  SupportsAVX = true;
#else
  SupportsSVE = Features.Has(vixl::CPUFeatures::Feature::kSVE);
  SupportsAVX = Features.Has(vixl::CPUFeatures::Feature::kSVE2) &&
                vixl::aarch64::CPU::ReadSVEVectorLengthInBits() >= 256;
#endif
  SupportsSHA = true;
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
  __asm volatile ("mrs %[ctr], ctr_el0"
    : [ctr] "=r"(CTR));

  DCacheLineSize = 4 << ((CTR >> 16) & 0xF);
  ICacheLineSize = 4 << (CTR & 0xF);

  // Test if this CPU supports float exception trapping by attempting to enable
  // On unsupported these bits are architecturally defined as RAZ/WI
  constexpr uint32_t ExceptionEnableTraps =
    (1U << 8) |  // Invalid Operation float exception trap enable
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
#endif

#endif
#if defined(_M_X86_64) && !defined(VIXL_SIMULATOR)
  Xbyak::util::Cpu Features{};
  SupportsAES = Features.has(Xbyak::util::Cpu::tAESNI);
  SupportsCRC = Features.has(Xbyak::util::Cpu::tSSE42);
  SupportsRAND = Features.has(Xbyak::util::Cpu::tRDRAND) && Features.has(Xbyak::util::Cpu::tRDSEED);
  SupportsRCPC = true;
  SupportsTSOImm9 = true;
  Supports3DNow = Features.has(Xbyak::util::Cpu::t3DN) && Features.has(Xbyak::util::Cpu::tE3DN);
  SupportsSSE4A = Features.has(Xbyak::util::Cpu::tSSE4a);
  SupportsAVX = true;
  SupportsSHA = Features.has(Xbyak::util::Cpu::tSHA);
  SupportsBMI1 = Features.has(Xbyak::util::Cpu::tBMI1);
  SupportsBMI2 = Features.has(Xbyak::util::Cpu::tBMI2);
  SupportsBMI2 = Features.has(Xbyak::util::Cpu::tCLWB);
  SupportsPMULL_128Bit = Features.has(Xbyak::util::Cpu::tPCLMULQDQ);

  // xbyak doesn't know how to check for CLZero
  // First ensure we support a new enough extended CPUID function range

  uint32_t data[4];
  Xbyak::util::Cpu::getCpuid(0x8000'0000, data);
  if (data[0] >= 0x8000'0008U) {
    // CLZero defined in 8000_00008_EBX[bit 0]
    Xbyak::util::Cpu::getCpuid(0x8000'0008, data);
    SupportsCLZERO = data[1] & 1;
  }

  SupportsFlushInputsToZero = true;
  SupportsFloatExceptions = true;
#endif

#ifdef VIXL_SIMULATOR
  // simulator doesn't support dc(ZVA)
  SupportsCLZERO = false;
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
  OverrideFeatures(this);
}
}
