#pragma once

#include "Interface/Core/Dispatcher/Dispatcher.h"

#define XBYAK64
#include <xbyak/xbyak.h>

namespace FEXCore::Core {
  struct InternalThreadState;
}

namespace FEXCore::CPU {

class X86Dispatcher final : public Dispatcher, public Xbyak::CodeGenerator {
  public:
    X86Dispatcher(FEXCore::Context::ContextImpl *ctx, const DispatcherConfig &config);
    void InitThreadPointers(FEXCore::Core::InternalThreadState *Thread) override;
    size_t GenerateGDBPauseCheck(uint8_t *CodeBuffer, uint64_t GuestRIP) override;
    size_t GenerateInterpreterTrampoline(uint8_t *CodeBuffer) override;

    virtual ~X86Dispatcher() override;
};

}
