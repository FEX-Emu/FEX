#pragma once

#include "Interface/Core/InternalThreadState.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/fextl/vector.h>

namespace FEXCore::CPU {
class Dispatcher;
class X86DispatchGenerator;
class Arm64DispatchGenerator;

#define DESTMAP_AS_MAP 0
#if DESTMAP_AS_MAP
using DestMapType = std::unordered_map<uint32_t, uint32_t>;
#else
using DestMapType = fextl::vector<uint32_t>;
#endif

class InterpreterCore final : public CPUBackend {
public:
  explicit InterpreterCore(Dispatcher *Dispatch,
                           FEXCore::Core::InternalThreadState *Thread);

  [[nodiscard]] std::string GetName() override { return "Interpreter"; }

  [[nodiscard]] CPUBackend::CompiledCode CompileCode(uint64_t Entry,
                                  FEXCore::IR::IRListView const *IR,
                                  FEXCore::Core::DebugData *DebugData,
                                  FEXCore::IR::RegisterAllocationData *RAData, bool GDBEnabled) override;

  [[nodiscard]] void *MapRegion(void* HostPtr, uint64_t, uint64_t) override { return HostPtr; }

  [[nodiscard]] bool NeedsOpDispatch() override { return true; }

  static void InitializeSignalHandlers(FEXCore::Context::ContextImpl *CTX);
  
  void ClearCache() override;

private:
  size_t BufferUsed;
  Dispatcher *Dispatch;
};

template<typename T>
T AtomicCompareAndSwap(T expected, T desired, T *addr);

uint8_t AtomicFetchNeg(uint8_t *Addr);
uint16_t AtomicFetchNeg(uint16_t *Addr);
uint32_t AtomicFetchNeg(uint32_t *Addr);
uint64_t AtomicFetchNeg(uint64_t *Addr);

} // namespace FEXCore::CPU
