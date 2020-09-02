#pragma once

#include "Interface/Core/BlockCache.h"
#include "Interface/Core/InternalThreadState.h"

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>

namespace FEXCore::CPU {
class DispatchGenerator;

#define DESTMAP_AS_MAP 0
#if DESTMAP_AS_MAP
using DestMapType = std::unordered_map<uint32_t, uint32_t>;
#else
using DestMapType = std::vector<uint32_t>;
#endif

class InterpreterCore final : public CPUBackend {
public:
  explicit InterpreterCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread);
  ~InterpreterCore() override;
  std::string GetName() override { return "Interpreter"; }
  void *CompileCode(FEXCore::IR::IRListView<true> const *IR, FEXCore::Core::DebugData *DebugData) override;

  void *MapRegion(void* HostPtr, uint64_t, uint64_t) override { return HostPtr; }

  bool NeedsOpDispatch() override { return true; }

  void ExecuteCode(FEXCore::Core::InternalThreadState *Thread);

  void CreateAsmDispatch(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread);
  void DeleteAsmDispatch();

private:
  FEXCore::Context::Context *CTX;
  FEXCore::Core::InternalThreadState *State;

  uint32_t AllocateTmpSpace(size_t Size);
  bool HandleSignalPause(int Signal, void *info, void *ucontext);
  bool HandleGuestSignal(int Signal, void *info, void *ucontext, SignalDelegator::GuestSigAction *GuestAction, stack_t *GuestStack);

  template<typename Res>
  Res GetDest(IR::OrderedNodeWrapper Op);

  template<typename Res>
  Res GetSrc(IR::OrderedNodeWrapper Src);

  std::vector<uint8_t> TmpSpace;
  DestMapType DestMap;
  size_t TmpOffset{};

  FEXCore::IR::IRListView<true> *CurrentIR;

  DispatchGenerator *Generator{};
};

}
