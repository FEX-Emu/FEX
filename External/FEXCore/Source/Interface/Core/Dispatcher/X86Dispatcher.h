#pragma once

#include "Interface/Core/Dispatcher/Dispatcher.h"

#include <FEXCore/Utils/Allocator.h>

#define XBYAK64
#include <xbyak/xbyak.h>

namespace FEXCore::CPU {

class X86Dispatcher final : public Dispatcher, public Xbyak::CodeGenerator {
  public:
    X86Dispatcher(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, DispatcherConfig &config);

    virtual ~X86Dispatcher() override;
};

}
