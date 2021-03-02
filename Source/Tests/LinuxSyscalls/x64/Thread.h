#pragma once
#include <stdint.h>

namespace FEXCore::Core {
struct InternalThreadState;
struct CpuStateFrame;
}

namespace FEX::HLE::x64 {
  uint64_t SetThreadArea(FEXCore::Core::InternalThreadState *Thread, void *tls);
  void AdjustRipForNewThread(FEXCore::Core::CpuStateFrame *Frame);
}
