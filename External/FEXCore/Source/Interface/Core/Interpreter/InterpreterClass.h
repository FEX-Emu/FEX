#pragma once

#include "Interface/Core/LookupCache.h"
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
  explicit InterpreterCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, bool CompileThread);
  std::string GetName() override { return "Interpreter"; }
  void *CompileCode(uint64_t Entry, FEXCore::IR::IRListView const *IR, FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData) override;

  void *MapRegion(void* HostPtr, uint64_t, uint64_t) override { return HostPtr; }

  bool NeedsOpDispatch() override { return true; }

  void CreateAsmDispatch(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread);

  bool HandleSIGBUS(int Signal, void *info, void *ucontext);

private:
  FEXCore::Context::Context *CTX;
  FEXCore::Core::InternalThreadState *State;

  uint32_t AllocateTmpSpace(size_t Size);

  template<typename Res>
  Res GetDest(void* SSAData, IR::OrderedNodeWrapper Op);

  template<typename Res>
  Res GetSrc(void* SSAData, IR::OrderedNodeWrapper Src);

  std::unique_ptr<Dispatcher> Dispatcher{};
};

}
