#pragma once

#include "Interface/Core/ArchHelpers/RISCVEmitter.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"

namespace FEXCore::Context {
struct Context;
}

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::CPU {

class RISCVDispatcher final : public Dispatcher, public RISCVEmitter {
  public:
    RISCVDispatcher(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, uint8_t *Buffer);

    std::array<__uint128_t, 64> FPRWorkingSpace{};
};

}
