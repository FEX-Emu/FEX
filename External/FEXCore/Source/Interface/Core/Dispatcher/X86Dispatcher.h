#pragma once

#include "Interface/Core/ArchHelpers/X86Emitter.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"

namespace FEXCore::CPU {
class X86Dispatcher final : public Dispatcher, public X86Emitter {
  public:
    X86Dispatcher(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, DispatcherConfig &config);

    virtual ~X86Dispatcher() override;
};

}