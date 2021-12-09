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
#endif
#ifdef _M_X86_64
  Xbyak::util::Cpu Features{};
  SupportsAES = Features.has(Xbyak::util::Cpu::tAESNI);
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
