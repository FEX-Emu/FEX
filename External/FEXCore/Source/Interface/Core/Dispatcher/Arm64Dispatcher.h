#pragma once

#include "Interface/Core/ArchHelpers/Arm64Emitter.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"

#include "aarch64/assembler-aarch64.h"

namespace FEXCore::CPU {

class Arm64Dispatcher final : public Dispatcher, public Arm64Emitter {
  public:
    Arm64Dispatcher(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, DispatcherConfig &config);

  protected:
    void SpillSRA(void *ucontext) override;
};

}