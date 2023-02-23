#pragma once

#include "Interface/Core/ArchHelpers/Arm64Emitter.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"

#ifdef VIXL_SIMULATOR
#include <aarch64/simulator-aarch64.h>
#endif

namespace FEXCore::Core {
struct InternalThreadState;
}

#define STATE_PTR(STATE_TYPE, FIELD) \
  STATE.R(), offsetof(FEXCore::Core::STATE_TYPE, FIELD)

namespace FEXCore::CPU {

class Arm64Dispatcher final : public Dispatcher, public Arm64Emitter {
  public:
    Arm64Dispatcher(FEXCore::Context::ContextImpl *ctx, const DispatcherConfig &config);
    void InitThreadPointers(FEXCore::Core::InternalThreadState *Thread) override;
    size_t GenerateGDBPauseCheck(uint8_t *CodeBuffer, uint64_t GuestRIP) override;
    size_t GenerateInterpreterTrampoline(uint8_t *CodeBuffer) override;

#ifdef VIXL_SIMULATOR
  void ExecuteDispatch(FEXCore::Core::CpuStateFrame *Frame) override;
  void ExecuteJITCallback(FEXCore::Core::CpuStateFrame *Frame, uint64_t RIP) override;
#endif

  void EmitDispatcher();

  FEXCore::Context::Context::JITRegionPairs GetDispatcherRegion() const override;

  protected:
    void SpillSRA(FEXCore::Core::InternalThreadState *Thread, void *ucontext, uint32_t IgnoreMask) override;

  private:
    // Long division helpers
    uint64_t LUDIVHandlerAddress{};
    uint64_t LDIVHandlerAddress{};
    uint64_t LUREMHandlerAddress{};
    uint64_t LREMHandlerAddress{};

#ifdef VIXL_SIMULATOR
    vixl::aarch64::Decoder Decoder;
    vixl::aarch64::Simulator Simulator;
#endif
};

}
