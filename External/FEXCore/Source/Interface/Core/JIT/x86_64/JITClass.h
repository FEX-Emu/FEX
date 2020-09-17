#pragma once

#include "Interface/Core/BlockCache.h"
#include "Interface/Core/BlockSamplingData.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include "Interface/Core/JIT/x86_64/JIT.h"
#include "Common/MathUtils.h"

#include <xbyak/xbyak.h>
using namespace Xbyak;

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>

namespace FEXCore::CPU {
struct CodeBuffer {
  uint8_t *Ptr;
  size_t Size;
};

CodeBuffer AllocateNewCodeBuffer(size_t Size);
void FreeCodeBuffer(CodeBuffer Buffer);

}

namespace FEXCore::CPU {


// Temp registers
// rax, rcx, rdx, rsi, r8, r9,
// r10, r11
//
// Callee Saved
// rbx, rbp, r12, r13, r14, r15
//
// 1St Argument: rdi <ThreadState>
// XMM:
// All temp
#define STATE r14
#define TMP1 rax
#define TMP2 rcx
#define TMP3 rdx
#define TMP4 rdi
#define TMP5 rbx
using namespace Xbyak::util;
const std::array<Xbyak::Reg, 9> RA64 = { rsi, r8, r9, r10, r11, rbp, r12, r13, r15 };
const std::array<std::pair<Xbyak::Reg, Xbyak::Reg>, 4> RA64Pair = {{ {rsi, r8}, {r9, r10}, {r11, rbp}, {r12, r13} }};
const std::array<Xbyak::Reg, 11> RAXMM = { xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm9, xmm10 };
const std::array<Xbyak::Xmm, 11> RAXMM_x = { xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm9, xmm10 };

class JITCore final : public CPUBackend, public Xbyak::CodeGenerator {
public:
  explicit JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, CodeBuffer Buffer);
  ~JITCore() override;
  std::string GetName() override { return "JIT"; }
  void *CompileCode(FEXCore::IR::IRListView<true> const *IR, FEXCore::Core::DebugData *DebugData) override;

  void *MapRegion(void* HostPtr, uint64_t, uint64_t) override { return HostPtr; }

  bool NeedsOpDispatch() override { return true; }

  void ClearCache() override;

  static constexpr size_t INITIAL_CODE_SIZE = 1024 * 1024 * 256;

  bool HandleSIGILL(int Signal, void *info, void *ucontext);
  bool HandleSignalPause(int Signal, void *info, void *ucontext);
  bool HandleGuestSignal(int Signal, void *info, void *ucontext, SignalDelegator::GuestSigAction *GuestAction, stack_t *GuestStack);

private:
  FEXCore::Context::Context *CTX;
  FEXCore::Core::InternalThreadState *ThreadState;
  std::unordered_map<IR::OrderedNodeWrapper::NodeOffsetType, Label> JumpTargets;

  bool MemoryDebug = false;

  /**
   * @name Register Allocation
   * @{ */
  constexpr static uint32_t NumGPRs = RA64.size(); // 4 is the minimum required for GPR ops
  constexpr static uint32_t NumXMMs = RAXMM.size();
  constexpr static uint32_t NumGPRPairs = RA64Pair.size();
  constexpr static uint32_t RegisterCount = NumGPRs + NumXMMs + NumGPRPairs;
  constexpr static uint32_t RegisterClasses = 3;

  constexpr static uint64_t GPRBase = (0ULL << 32);
  constexpr static uint64_t XMMBase = (1ULL << 32);
  constexpr static uint64_t GPRPairBase = (2ULL << 32);

  /**  @} */

  constexpr static uint8_t RA_8 = 0;
  constexpr static uint8_t RA_16 = 1;
  constexpr static uint8_t RA_32 = 2;
  constexpr static uint8_t RA_64 = 3;
  constexpr static uint8_t RA_XMM = 4;

  uint32_t GetPhys(uint32_t Node);

  template<uint8_t RAType>
  Xbyak::Reg GetSrc(uint32_t Node);
  template<uint8_t RAType>
  std::pair<Xbyak::Reg, Xbyak::Reg> GetSrcPair(uint32_t Node);

  template<uint8_t RAType>
  Xbyak::Reg GetDst(uint32_t Node);

  Xbyak::Xmm GetSrc(uint32_t Node);
  Xbyak::Xmm GetDst(uint32_t Node);

  void CreateCustomDispatch(FEXCore::Core::InternalThreadState *Thread);
  IR::RegisterAllocationPass *RAPass;

  void *InterpreterFallbackHelperAddress;

#ifdef BLOCKSTATS
  bool GetSamplingData {true};
#endif
  static constexpr size_t MAX_CODE_SIZE = 1024 * 1024 * 256;

  static constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096 * 2;

  void EmplaceNewCodeBuffer(CodeBuffer Buffer) {
    CurrentCodeBuffer = &CodeBuffers.emplace_back(Buffer);
  }

  // This is the initial code buffer that we will fall back to
  // In a program without signals and code clearing, we will typically
  // only have this code buffer
  CodeBuffer InitialCodeBuffer{};
  // This is the array of /additional/ code buffers that we may need to allocate
  // Allocation only occurs when we've hit signals and need to clear code cache
  // For code safety we can't delete code buffers until outside of all signals
  std::vector<CodeBuffer> CodeBuffers{};

  // This is the codebuffer that our dispatcher lives in
  CodeBuffer DispatcherCodeBuffer{};
  // This is the current code buffer that we are tracking
  CodeBuffer *CurrentCodeBuffer{};

  uint64_t AbsoluteLoopTopAddress{};
  uint64_t ThreadStopHandlerAddress{};
  uint64_t ThreadPauseHandlerAddress{};
  Label ThreadPauseHandler{};

  uint64_t SignalHandlerReturnAddress{};
  uint64_t PauseReturnInstruction{};

  uint32_t SignalHandlerRefCounter{};

  void StoreThreadState(int Signal, void *ucontext);
  void RestoreThreadState(void *ucontext);
  std::stack<uint64_t> SignalFrames;
};

}
