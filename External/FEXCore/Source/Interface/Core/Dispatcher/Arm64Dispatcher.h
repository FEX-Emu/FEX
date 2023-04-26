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

  uint16_t GetSRAGPRCount() const override {
    return SRA64.size();
  }

  uint16_t GetSRAFPRCount() const override {
    return SRAFPR.size();
  }

  void GetSRAGPRMapping(uint8_t Mapping[16]) const override {
    for (size_t i = 0; i < SRA64.size(); ++i) {
      Mapping[i] = SRA64[i].Idx();
    }
  }

  void GetSRAFPRMapping(uint8_t Mapping[16]) const override {
    for (size_t i = 0; i < SRAFPR.size(); ++i) {
      Mapping[i] = SRAFPR[i].Idx();
    }
  }

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
