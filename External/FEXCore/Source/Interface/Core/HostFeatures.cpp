#include "Interface/Core/CPUID.h"
#include "Interface/Core/HostFeatures.h"

#ifdef _M_ARM_64
#include "aarch64/assembler-aarch64.h"
#include "aarch64/cpu-aarch64.h"
#include "aarch64/disasm-aarch64.h"
#include "aarch64/assembler-aarch64.h"
#endif

#ifdef _M_X86_64
#include <xbyak/xbyak_util.h>
#endif

namespace FEXCore {

// Data Zero Prohibited flag
// 0b0 = ZVA/GVA/GZVA permitted
// 0b1 = ZVA/GVA/GZVA prohibited
constexpr uint32_t DCZID_DZP_MASK = 0b1'0000;
// Log2 of the blocksize in 32-bit words
constexpr uint32_t DCZID_BS_MASK = 0b0'1111;

#ifdef _M_ARM_64
static uint32_t GetDCZID() {
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


HostFeatures::HostFeatures() {
#ifdef _M_ARM_64
  auto Features = vixl::CPUFeatures::InferFromOS();
  SupportsAES = Features.Has(vixl::CPUFeatures::Feature::kAES);
  SupportsCRC = Features.Has(vixl::CPUFeatures::Feature::kCRC32);
  SupportsAtomics = Features.Has(vixl::CPUFeatures::Feature::kAtomics);
  // Only supported when FEAT_AFP is supported
  SupportsFlushInputsToZero = Features.Has(vixl::CPUFeatures::Feature::kAFP);

  // RCPC is bugged on Snapdragon 865
  // Causes glibc cond16 test to immediately throw assert
  // __pthread_mutex_cond_lock: Assertion `mutex->__data.__owner == 0'
  SupportsRCPC = false; //Features.Has(vixl::CPUFeatures::Feature::kRCpc);

  // We need to get the CPU's cache line size
  // We expect sane targets that have correct cacheline sizes across clusters
  uint64_t CTR;
  __asm volatile ("mrs %[ctr], ctr_el0"
    : [ctr] "=r"(CTR));

  DCacheLineSize = 4 << ((CTR >> 16) & 0xF);
  ICacheLineSize = 4 << (CTR & 0xF);

  if (!SupportsAtomics) {
    WARN_ONCE_FMT("Host CPU doesn't support atomics. Expect bad performance");
  }
#endif
#ifdef _M_X86_64
  Xbyak::util::Cpu Features{};
  SupportsAES = Features.has(Xbyak::util::Cpu::tAESNI);
  SupportsCRC = Features.has(Xbyak::util::Cpu::tSSE42);
  SupportsFlushInputsToZero = true;
  SupportsFloatExceptions = true;
#else
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

  // Check if we can support cacheline clears
  uint32_t DCZID = GetDCZID();
  if ((DCZID & DCZID_DZP_MASK) == 0) {
    uint32_t DCZID_Log2 = DCZID & DCZID_BS_MASK;
    uint32_t DCZID_Bytes = (1 << DCZID_Log2) * sizeof(uint32_t);
    // If the DC ZVA size matches the emulated cache line size
    // This means we can use the instruction
    SupportsCLZERO = DCZID_Bytes == CPUIDEmu::CACHELINE_SIZE;
  }
}
}
