#pragma once
#include <stdint.h>

namespace FEXCore::Core {
struct InternalThreadState;
struct CPUState;
}

namespace FEX::HLE::x64 {
  uint64_t SetThreadArea(FEXCore::Core::InternalThreadState *Thread, void *tls);
  void AdjustRipForNewThread(FEXCore::Core::CPUState *Thread);
}
