#pragma once

#include "Interface/Core/ArchHelpers/Arm64Emitter.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"

namespace FEXCore::Context {
struct Context;
}

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::CPU {

class Arm64Dispatcher final : public Dispatcher, public Arm64Emitter {
  public:
    Arm64Dispatcher(FEXCore::Context::Context *ctx, const DispatcherConfig &config);
    void InitThreadPointers(FEXCore::Core::InternalThreadState *Thread) override;
    size_t GenerateGDBPauseCheck(uint8_t *CodeBuffer, uint64_t GuestRIP) override;
    size_t GenerateInterpreterTrampoline(uint8_t *CodeBuffer) override;

  protected:
    void SpillSRA(FEXCore::Core::InternalThreadState *Thread, void *ucontext, uint32_t IgnoreMask) override;

  private:
    // Long division helpers
    uint64_t LUDIVHandlerAddress{};
    uint64_t LDIVHandlerAddress{};
    uint64_t LUREMHandlerAddress{};
    uint64_t LREMHandlerAddress{};
    uint64_t StaticRegsSpillerAddress{};
    uint64_t StaticRegsFillerAddress{};
    DispatcherConfig config;
};

}
