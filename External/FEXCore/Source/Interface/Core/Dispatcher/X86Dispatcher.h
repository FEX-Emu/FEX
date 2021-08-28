#pragma once

#include "Interface/Core/Dispatcher/Dispatcher.h"

#define XBYAK64
#include <xbyak/xbyak.h>

namespace FEXCore::Context {
  struct Context;
}

namespace FEXCore::Core {
  struct InternalThreadState;
}

namespace FEXCore::CPU {

class X86Dispatcher final : public Dispatcher, public Xbyak::CodeGenerator {
  public:
    X86Dispatcher(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, DispatcherConfig &config);

    virtual ~X86Dispatcher() override;
};

}
