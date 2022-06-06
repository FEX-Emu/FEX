#pragma once

#include "Interface/Core/InternalThreadState.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>

namespace FEXCore::CPU {
class X86DispatchGenerator;
class Arm64DispatchGenerator;

#define DESTMAP_AS_MAP 0
#if DESTMAP_AS_MAP
using DestMapType = std::unordered_map<uint32_t, uint32_t>;
#else
using DestMapType = std::vector<uint32_t>;
#endif

class InterpreterCore final : public CPUBackend {
public:
  explicit InterpreterCore(FEXCore::Context::Context *ctx,
                           FEXCore::Core::InternalThreadState *Thread);

  [[nodiscard]] std::string GetName() override { return "Interpreter"; }

  [[nodiscard]] void *CompileCode(uint64_t Entry,
                                  FEXCore::IR::IRListView const *IR,
                                  FEXCore::Core::DebugData *DebugData,
                                  FEXCore::IR::RegisterAllocationData *RAData) override;

  [[nodiscard]] void *MapRegion(void* HostPtr, uint64_t, uint64_t) override { return HostPtr; }

  [[nodiscard]] bool NeedsOpDispatch() override { return true; }

  void CreateAsmDispatch(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread);

  static void InitializeSignalHandlers(FEXCore::Context::Context *CTX);

  bool NeedsRetainedIRCopy() const override { return true; }

private:
  FEXCore::Context::Context *CTX;
};

template<typename T>
T AtomicCompareAndSwap(T expected, T desired, T *addr);

uint8_t AtomicFetchNeg(uint8_t *Addr);
uint16_t AtomicFetchNeg(uint16_t *Addr);
uint32_t AtomicFetchNeg(uint32_t *Addr);
uint64_t AtomicFetchNeg(uint64_t *Addr);

} // namespace FEXCore::CPU
