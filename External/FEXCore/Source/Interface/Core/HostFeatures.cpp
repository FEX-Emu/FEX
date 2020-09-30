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

HostFeatures::HostFeatures() {
#ifdef _M_ARM_64
  auto Features = vixl::CPUFeatures::InferFromOS();
  SupportsAES = Features.Has(vixl::CPUFeatures::Feature::kAES);
#endif
#ifdef _M_X86_64
  Xbyak::util::Cpu Features{};
  SupportsAES = Features.has(Xbyak::util::Cpu::tAESNI);
#endif
}
}
