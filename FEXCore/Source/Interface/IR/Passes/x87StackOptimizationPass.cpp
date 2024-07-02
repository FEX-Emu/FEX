#include "FEXCore/Utils/LogManager.h"
#include "Interface/IR/IR.h"
#include "Interface/IR/IREmitter.h"
#include "Interface/IR/PassManager.h"
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/deque.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdint.h>

namespace FEXCore::IR {

// FixedSizeStack is a model of the x87 Stack where each element in this
// fixed size stack lives at an offset from top. The top of the stack is at
// index 0.
template<typename T, size_t MaxSize>
class FixedSizeStack {
public:
  enum class Slot { UNUSED, INVALID, VALID };
  FixedSizeStack() {
    buffer.resize(MaxSize, {Slot::UNUSED, T()});
  }

  void push(const T& value) {
    rotate();
    buffer.front() = {Slot::VALID, value};
  }

  // Rotate the elements with the direction controlled by Right
  void rotate(bool Right = true) {
    if (Right) {
      // Right rotation
      std::pair<Slot, T> temp = buffer.back();
      buffer.pop_back();
      buffer.push_front(temp);
      TopOffset++;
    } else {
      // Left rotation
      std::pair<Slot, T> temp = buffer.front();
      buffer.pop_front();
      buffer.push_back(temp);
      TopOffset--;
    }
  }

  void pop() {
    buffer.front() = {Slot::INVALID, T()};
    rotate(false);
  }

  const std::pair<Slot, T>& top(size_t Offset = 0) const {
    return buffer[Offset];
  }

  void setTop(T Value, size_t Offset = 0) {
    buffer[Offset] = {Slot::VALID, Value};
  }

  bool isValid(size_t Offset) const {
    return buffer[Offset].first;
  }

  inline void clear() {
    for (auto& element : buffer) {
      element = {Slot::UNUSED, T()}; // Set all elements as invalid
    }
    TopOffset = 0;
  }

  void dump() const {
    for (size_t i = 0; i < MaxSize; i++) {
      const auto& [Valid, Element] = buffer[i];
      if (Valid == Slot::VALID) {
        LogMan::Msg::DFmt("ST{}: 0x{:x}", i, (uintptr_t)(Element.StackDataNode));
      }
    }
  }

  constexpr size_t size() const {
    return MaxSize;
  }

  int8_t getTopOffset() const {
    return TopOffset;
  }

  void setTopOffset(int8_t Offset) {
    TopOffset = Offset;
  }

  void setValidTag(size_t Index, bool Valid) {
    buffer[Index].first = Valid ? Slot::VALID : Slot::INVALID;
  }

  // Returns a mask to set in AbridgedTagWord
  uint8_t getValidMask() {
    uint8_t Mask = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
      const bool Valid = buffer[i].first == Slot::VALID;
      if (Valid) {
        Mask |= 1U << i;
      }
    }
    return Mask;
  }

  // Returns a mask to set in AbridgedTagWord
  uint8_t getInvalidMask() {
    uint8_t Mask = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
      const bool Valid = buffer[i].first == Slot::INVALID;
      if (Valid) {
        Mask |= 1U << i;
      }
    }
    return Mask;
  }

private:
  fextl::deque<std::pair<Slot, T>> buffer;
  // Real top as an offset from stored top value (or the one at the beginning of the block)
  // For example, if we start and push a value to our simulated stack, because we don't
  // update top straight away the TopOffset is 1.
  // If SlowPath is true, then TopOffset is always zero.
  int8_t TopOffset = 0;
};


class X87StackOptimization final : public FEXCore::IR::Pass {
public:
  X87StackOptimization() {
    FEX_CONFIG_OPT(ReducedPrecision, X87REDUCEDPRECISION);
    ReducedPrecisionMode = ReducedPrecision;
  }
  void Run(IREmitter* Emit) override;

private:
  bool ReducedPrecisionMode;
  // FIXME(pmatos): copy from OpcodeDispatcher.h
  [[nodiscard]]
  uint32_t MMBaseOffset() {
    return static_cast<uint32_t>(offsetof(Core::CPUState, mm[0][0]));
  }
  std::tuple<Ref, Ref> SplitF64SigExp(Ref Node);

  // Top Management Helpers
  /// Set the valid tag for Value as valid (if Valid is true), or invalid (if Valid is false).
  void SetX87ValidTag(Ref Value, bool Valid);
  // Generates slow code to load/store a value from the top of the stack
  Ref LoadStackValueAtTop_Slow();
  void StoreStackValueAtTop_Slow(Ref Value, bool SetValid = true);
  // Generates slow code to load/store a value from an offset from the top of the stack
  Ref LoadStackValueAtOffset_Slow(uint8_t Offset);
  void StoreStackValueAtOffset_Slow(uint8_t Offset, Ref Value, bool SetValid = true);
  // Update Top value in slow path for a pop
  void UpdateTop4Pop_Slow();
  void UpdateTop4Push_Slow();
  // Synchronizes the current simulated stack with the actual values.
  // Returns a new value for Top, that's synchronized between the simulated stack
  // and the actual FPU stack.
  Ref SynchronizeStackValues();
  // Moves us from the fast to the slow path if ShouldMigrate is true.
  void MigrateToSlowPathIf(bool ShouldMigrate);
  // Top Cache Management
  Ref GetTopWithCache_Slow();
  Ref GetOffsetTopWithCache_Slow(uint8_t Offset);
  void SetTopWithCache_Slow(Ref Value);
  Ref GetX87ValidTag_Slow(uint8_t Offset);
  // Resets fields to initial values
  void Reset(bool AlsoSlowPath = true);

  struct StackMemberInfo {
    IR::OpSize SourceDataSize; // Size of SourceDataNode
    IR::OpSize StackDataSize;  // Size of the data in the stack
    IR::Ref SourceDataNode;    // Reference to the source location of the data.
                               // In the case of a load, this is the source node of the load.
                               // If it's not a load, then it's nullptr.
    IR::Ref StackDataNode;     // Reference to the data in the Stack.
    bool InterpretAsFloat {};  // True if this is a floating point value, false if integer
  };

  // StackData, TopCache need to be always properly set to ensure
  // it reflects the current state of the FPU. This sync only makes sense while
  // taking the fast path. Once in the slow path, these don't make sense anymore
  // and we are syncing everything.

  // Index on vector is offset to top value at start of block
  // If slow path is true, then StackData is always empty.
  FixedSizeStack<StackMemberInfo, 8> StackData;
  using StackSlot = FixedSizeStack<StackMemberInfo, 8>::Slot;

  void InvalidateCaches();
  void InvalidateTopOffsetCache();

  // Cache for Constants
  // ConstantPoll[i] has IREmit->_Constant(i);
  constexpr static size_t ConstantPoolSize = 8;
  std::array<Ref, ConstantPoolSize> ConstantPool;
  Ref GetConstant(ssize_t Offset);

  // Cached value for Top
  // If slowpath is false, then TopCache is nullptr.
  std::array<Ref, 8> TopOffsetCache;
  // Are we on the slow path?
  // Once we enter the slow path, we never come out.
  // This just simplifies the code atm. If there's a need to return to the fast path in the future
  // we can implement that but I would expect that there would be very few cases where that's necessary.
  // On the slow path TopCache is always the last obtained version of top.
  // TopOffset is ignored
  bool SlowPath = false;
  // Keeping IREmitter not to pass arguments around
  IREmitter* IREmit = nullptr;
};

inline void X87StackOptimization::InvalidateCaches() {
  LogMan::Msg::DFmt("Invalidating caches");

  InvalidateTopOffsetCache();
  ConstantPool.fill(nullptr);
}

inline void X87StackOptimization::InvalidateTopOffsetCache() {
  TopOffsetCache.fill(nullptr);
}

inline void X87StackOptimization::Reset(bool AlsoSlowPath) {
  if (AlsoSlowPath) {
    SlowPath = false;
  }
  StackData.clear();
  InvalidateCaches();
}

inline Ref X87StackOptimization::GetConstant(ssize_t Offset) {
  if (Offset < 0 || Offset >= X87StackOptimization::ConstantPoolSize) {
    // not dealt by pool
    LogMan::Msg::DFmt("Generating uncacheable constant for {}", Offset);
    return IREmit->_Constant(Offset);
  }
  if (ConstantPool[Offset] == nullptr) {
    LogMan::Msg::DFmt("Generating and caching constant for {}", Offset);

    ConstantPool[Offset] = IREmit->_Constant(Offset);
  } else {
    LogMan::Msg::DFmt("Returning cached constant for {}", Offset);
  }
  return ConstantPool[Offset];
}

inline void X87StackOptimization::MigrateToSlowPathIf(bool ShouldMigrate) {
  if (SlowPath) {
    return;
  }
  if (!ShouldMigrate) {
    return;
  }

  LogMan::Msg::DFmt("Migrating to SlowPath");
  SynchronizeStackValues();
  Reset(false); // Reset everything but no need to change slowpath
  SlowPath = true;
}

inline Ref X87StackOptimization::GetTopWithCache_Slow() {
  if (!TopOffsetCache[0]) {
    TopOffsetCache[0] = IREmit->_LoadContext(1, GPRClass, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC);
  }
  return TopOffsetCache[0];
}

inline Ref X87StackOptimization::GetOffsetTopWithCache_Slow(uint8_t Offset) {
  if (TopOffsetCache[Offset]) {
    return TopOffsetCache[Offset];
  }

  auto* OffsetTop = GetTopWithCache_Slow();
  if (Offset != 0) {
    OffsetTop = IREmit->_And(OpSize::i32Bit, IREmit->_Add(OpSize::i32Bit, OffsetTop, GetConstant(Offset)), GetConstant(7));
    // GetTopWithCache_Slow already sets the cache so we don't need to set it here for offset == 0
    TopOffsetCache[Offset] = OffsetTop;
  }

  return OffsetTop;
}


inline void X87StackOptimization::SetTopWithCache_Slow(Ref Value) {
  IREmit->_StoreContext(1, GPRClass, Value, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC);
  InvalidateTopOffsetCache();
  TopOffsetCache[0] = Value;
}

inline void X87StackOptimization::SetX87ValidTag(Ref Value, bool Valid) {
  Ref AbridgedFTW = IREmit->_LoadContext(1, GPRClass, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
  Ref RegMask = IREmit->_Lshl(OpSize::i32Bit, GetConstant(1), Value);
  Ref NewAbridgedFTW = Valid ? IREmit->_Or(OpSize::i32Bit, AbridgedFTW, RegMask) : IREmit->_Andn(OpSize::i32Bit, AbridgedFTW, RegMask);
  IREmit->_StoreContext(1, GPRClass, NewAbridgedFTW, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
}

inline Ref X87StackOptimization::GetX87ValidTag_Slow(uint8_t Offset) {
  Ref AbridgedFTW = IREmit->_LoadContext(1, GPRClass, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
  return IREmit->_And(OpSize::i32Bit, IREmit->_Lshr(OpSize::i32Bit, AbridgedFTW, GetOffsetTopWithCache_Slow(Offset)), GetConstant(1));
}

inline Ref X87StackOptimization::LoadStackValueAtTop_Slow() {
  return LoadStackValueAtOffset_Slow(0);
}

inline Ref X87StackOptimization::LoadStackValueAtOffset_Slow(uint8_t Offset) {
  return IREmit->_LoadContextIndexed(GetOffsetTopWithCache_Slow(Offset), ReducedPrecisionMode ? 8 : 16, MMBaseOffset(), 16, FPRClass);
}

inline void X87StackOptimization::StoreStackValueAtTop_Slow(Ref Value, bool SetValid) {
  StoreStackValueAtOffset_Slow(0, Value, SetValid);
}

inline void X87StackOptimization::StoreStackValueAtOffset_Slow(uint8_t Offset, Ref Value, bool SetValid) {
  OrderedNode* TopOffset = GetOffsetTopWithCache_Slow(Offset);
  // store
  IREmit->_StoreContextIndexed(Value, TopOffset, ReducedPrecisionMode ? 8 : 16, MMBaseOffset(), 16, FPRClass);
  // mark it valid
  // In some cases we might already know it has been previously set as valid so we don't need to do it again
  if (SetValid) {
    SetX87ValidTag(TopOffset, true);
  }
}

inline void X87StackOptimization::UpdateTop4Pop_Slow() {
  // Pop the top of the x87 stack
  auto* TopOffset = GetTopWithCache_Slow();
  TopOffset = IREmit->_Add(OpSize::i32Bit, TopOffset, GetConstant(1));
  TopOffset = IREmit->_And(OpSize::i32Bit, TopOffset, GetConstant(7));
  SetTopWithCache_Slow(TopOffset);
}

inline void X87StackOptimization::UpdateTop4Push_Slow() {
  // Pop the top of the x87 stack
  auto* TopOffset = GetTopWithCache_Slow();
  TopOffset = IREmit->_Sub(OpSize::i32Bit, TopOffset, GetConstant(1));
  TopOffset = IREmit->_And(OpSize::i32Bit, TopOffset, GetConstant(7));
  SetTopWithCache_Slow(TopOffset);
}

// We synchronize stack values in a few occasions but one of the most important of those,
// is when we move from fast to a slow path and need to make sure that the context is properly
// written.
Ref X87StackOptimization::SynchronizeStackValues() {
  if (SlowPath) { // Nothing to do here.
    LogMan::Msg::DFmt("Sync called in slow path - nothing to do here");
    return GetTopWithCache_Slow();
  }

  // Store new top which is now the original top minus recorded top offset
  // Careful with underflow wraparound.
  const auto TopOffset = StackData.getTopOffset();
  LogMan::Msg::DFmt("Writing stack to context - topoffset: {}", TopOffset);

  if (TopOffset != 0) {
    auto* OrigTop = GetTopWithCache_Slow();
    Ref NewTop = nullptr;
    if (TopOffset > 0) {
      NewTop = IREmit->_And(OpSize::i32Bit, IREmit->_Sub(OpSize::i32Bit, OrigTop, GetConstant(TopOffset)), GetConstant(0x7));
    } else {
      NewTop = IREmit->_And(OpSize::i32Bit, IREmit->_Add(OpSize::i32Bit, OrigTop, GetConstant(-TopOffset)), GetConstant(0x7));
    }
    SetTopWithCache_Slow(NewTop);
  }
  StackData.setTopOffset(0);

  // Before leaving we need to write the current values in the stack to
  // context so that the values are correct. Copy SourceDataNode in the
  // stack to the respective mmX register.
  Ref TopValue = GetTopWithCache_Slow();
  StackData.dump();
  for (size_t i = 0; i < StackData.size(); ++i) {
    const auto& [Valid, StackMember] = StackData.top(i);

    if (Valid == StackSlot::UNUSED) {
      continue;
    }
    Ref TopIndex = GetOffsetTopWithCache_Slow(i);
    if (Valid == StackSlot::VALID) {
      LogMan::Msg::DFmt("Writing StackData[{}]", i);
      IREmit->_StoreContextIndexed(StackMember.StackDataNode, TopIndex, ReducedPrecisionMode ? 8 : 16, MMBaseOffset(), 16, FPRClass);
    }
  }
  { // Set valid tags
    uint8_t Mask = StackData.getValidMask();
    Ref MaskC = GetConstant(Mask);
    LogMan::Msg::DFmt("Writing valid tags 0x{:02x}", Mask);
    if (Mask == 0xff) {
      IREmit->_StoreContext(1, GPRClass, GetConstant(Mask), offsetof(FEXCore::Core::CPUState, AbridgedFTW));
    } else if (Mask != 0) {
      if (std::popcount(Mask) == 1) {
        uint8_t BitIdx = __builtin_ctz(Mask);
        SetX87ValidTag(GetOffsetTopWithCache_Slow(BitIdx), true);
      } else {
        // perform a rotate right on mask by top
        // since we must operate on 32bits as a minimum:
        // ror (mask, top) = ((M << 8) | M) >> TOP
        auto* TopValue = GetTopWithCache_Slow();
        Ref RotAmount = IREmit->_Sub(OpSize::i32Bit, GetConstant(8), TopValue);
        Ref RMask = IREmit->_Or(OpSize::i32Bit, MaskC, IREmit->_Lshl(OpSize::i32Bit, MaskC, GetConstant(8)));
        RMask = IREmit->_Lshr(OpSize::i32Bit, RMask, RotAmount);
        Ref AbridgedFTW = IREmit->_LoadContext(1, GPRClass, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
        Ref NewAbridgedFTW = IREmit->_Or(OpSize::i32Bit, AbridgedFTW, RMask);
        IREmit->_StoreContext(1, GPRClass, NewAbridgedFTW, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
      }
    } else {
      LogMan::Msg::DFmt("No valid tags written");
    }
  }
  { // Set invalid tags
    uint8_t Mask = StackData.getInvalidMask();
    Ref MaskC = GetConstant(Mask);
    LogMan::Msg::DFmt("Writing invalid tags 0x{:02x}", Mask);
    if (Mask == 0xff) {
      IREmit->_StoreContext(1, GPRClass, GetConstant(0), offsetof(FEXCore::Core::CPUState, AbridgedFTW));
    } else if (Mask != 0) {
      if (std::popcount(Mask)) {
        uint8_t BitIdx = __builtin_ctz(Mask);
        SetX87ValidTag(GetOffsetTopWithCache_Slow(BitIdx), false);
      } else {
        // Same rotate right as above but this time on the invalid mask
        auto* TopValue = GetTopWithCache_Slow();
        Ref RotAmount = IREmit->_Sub(OpSize::i32Bit, GetConstant(8), TopValue);
        Ref RMask = IREmit->_Or(OpSize::i32Bit, MaskC, IREmit->_Lshl(OpSize::i32Bit, MaskC, GetConstant(8)));
        RMask = IREmit->_Lshr(OpSize::i32Bit, RMask, RotAmount);
        Ref AbridgedFTW = IREmit->_LoadContext(1, GPRClass, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
        Ref NewAbridgedFTW = IREmit->_Andn(OpSize::i32Bit, AbridgedFTW, RMask);
        IREmit->_StoreContext(1, GPRClass, NewAbridgedFTW, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
      }
    } else {
      LogMan::Msg::DFmt("No invalid tags written");
    }
  }
  return TopValue;
}

std::tuple<Ref, Ref> X87StackOptimization::SplitF64SigExp(Ref Node) {
  Ref Gpr = IREmit->_VExtractToGPR(8, 8, Node, 0);

  Ref Exp = IREmit->_And(OpSize::i64Bit, Gpr, GetConstant(0x7ff0000000000000LL));
  Exp = IREmit->_Lshr(OpSize::i64Bit, Exp, GetConstant(52));
  Exp = IREmit->_Sub(OpSize::i64Bit, Exp, GetConstant(1023));
  Exp = IREmit->_Float_FromGPR_S(8, 8, Exp);
  Ref Sig = IREmit->_And(OpSize::i64Bit, Gpr, GetConstant(0x800fffffffffffffLL));
  Sig = IREmit->_Or(OpSize::i64Bit, Sig, GetConstant(0x3ff0000000000000LL));
  Sig = IREmit->_VCastFromGPR(8, 8, Sig);

  return std::tuple {Exp, Sig};
}

void X87StackOptimization::Run(IREmitter* Emit) {
  FEXCORE_PROFILE_SCOPED("PassManager::x87StackOpt");

  auto CurrentIR = Emit->ViewIR();
  auto* HeaderOp = CurrentIR.GetHeader();
  LOGMAN_THROW_AA_FMT(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  if (!HeaderOp->HasX87) {
    // If there is no x87 in this, just early exit.
    return;
  }

  // Initialize IREmit member
  IREmit = Emit;

  auto* OriginalWriteCursor = IREmit->GetWriteCursor();

  // Run optimization proper
  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
    // Each time we deal with a new block we need to start over.
    // The optimization should run per-block
    LogMan::Msg::DFmt("Starting new block - resetting and back to fast path");
    Reset();

    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      IREmit->SetWriteCursor(CodeNode);

      switch (IROp->Op) {

      case IR::OP_INITSTACK: {

        StackData.clear();
        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_INVALIDATESTACK: {


        LogMan::Msg::DFmt("OP_INVALIDATESTACK");
        const auto* Op = IROp->C<IR::IROp_ReadStackValue>();
        auto Offset = Op->StackLocation;

        if (Offset != 0xff) { // invalidate single offset
          if (SlowPath) {
            auto* TopValue = GetTopWithCache_Slow();
            if (Offset != 0) {
              auto* Mask = GetConstant(7);
              TopValue = IREmit->_And(OpSize::i32Bit, IREmit->_Add(OpSize::i32Bit, TopValue, GetConstant(Offset)), Mask);
            }
            SetX87ValidTag(TopValue, false);
          } else {
            StackData.setValidTag(Offset, false);
          }
        } else { // invalidate all
          if (SlowPath) {
            IREmit->_StoreContext(1, GPRClass, GetConstant(0), offsetof(FEXCore::Core::CPUState, AbridgedFTW));
          } else {
            for (size_t i = 0; i < StackData.size(); i++) {
              StackData.setValidTag(i, false);
            }
          }
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_PUSHSTACK: {


        const auto* Op = IROp->C<IR::IROp_PushStack>();
        auto* SourceNode = CurrentIR.GetNode(Op->X80Src);

        if (SlowPath) {
          LogMan::Msg::DFmt("OP_PUSHSTACK SlowPath");
          UpdateTop4Push_Slow();
          StoreStackValueAtTop_Slow(SourceNode);
        } else {
          LogMan::Msg::DFmt("OP_PUSHSTACK FastPath");
          auto* SourceNode = CurrentIR.GetNode(Op->X80Src);
          auto* SourceNodeOp = CurrentIR.GetOp<IROp_Header>(SourceNode);
          auto SourceNodeSize = SourceNodeOp->Size;
          StackData.push(StackMemberInfo {
            .SourceDataSize = IR::SizeToOpSize(SourceNodeSize),
            .StackDataSize = IR::SizeToOpSize(Op->LoadSize),
            .SourceDataNode = nullptr,
            .StackDataNode = SourceNode,
            .InterpretAsFloat = Op->Float,
          });
        }
        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_READSTACKVALUE: {


        const auto* Op = IROp->C<IR::IROp_ReadStackValue>();
        auto Offset = Op->StackLocation;
        const auto& [Valid, Value] = StackData.top(Offset);
        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        Ref NewValue = nullptr;
        if (SlowPath) {
          // slow path
          LogMan::Msg::DFmt("OP_READSTACKVALUE slow");
          NewValue = LoadStackValueAtOffset_Slow(Offset);
        } else { // fast path
          LogMan::Msg::DFmt("OP_READSTACKVALUE fast");
          NewValue = StackData.top(Offset).second.StackDataNode;
        }

        IREmit->ReplaceUsesWithAfter(CodeNode, NewValue, CodeNode);
        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_STACKVALIDTAG: {


        // Returns 0 if value is valid and 1 otherwise.
        const auto* Op = IROp->C<IR::IROp_StackValidTag>();
        auto Offset = Op->StackLocation;
        const auto& [Valid, Value] = StackData.top(Offset);
        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        Ref Tag = nullptr;

        if (SlowPath) {
          LogMan::Msg::DFmt("STACKVALIDTAG slow");
          Tag = GetX87ValidTag_Slow(Offset);
        } else {
          LogMan::Msg::DFmt("STACKVALIDTAG fast");
          Tag = StackData.top(Offset).first == StackSlot::VALID ? GetConstant(1) : GetConstant(0);
        }

        IREmit->ReplaceUsesWithAfter(CodeNode, Tag, CodeNode);
        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_STORESTACKMEMORY: { // stores top of stack in mem addr.


        LogMan::Msg::DFmt("OP_STORESTACKMEMORY");
        const auto* Op = IROp->C<IR::IROp_StoreStackMemory>();

        const auto& [Valid, Value] = StackData.top();
        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        Ref StackNode = nullptr;
        if (SlowPath) { // slow path
          LogMan::Msg::DFmt("Slow path STORESTACKMEMORY");
          StackNode = LoadStackValueAtTop_Slow();
        } else { // fast path
          LogMan::Msg::DFmt("Fast path STORESTACKMEMORY");
          StackNode = StackData.top().second.StackDataNode;
        }

        auto* AddrNode = CurrentIR.GetNode(Op->Addr);

        if (ReducedPrecisionMode) {
          switch (Op->StoreSize) {
          case 4: {
            StackNode = IREmit->_Float_FToF(4, 8, StackNode);
            IREmit->_StoreMem(FPRClass, 8, AddrNode, StackNode);
            break;
          }
          case 8: {
            IREmit->_StoreMem(FPRClass, 8, AddrNode, StackNode);
            break;
          }
          case 10: {
            StackNode = IREmit->_F80CVTTo(StackNode, 8);
            IREmit->_StoreMem(FPRClass, 8, AddrNode, StackNode);
            auto Upper = IREmit->_VExtractToGPR(16, 8, StackNode, 1);
            IREmit->_StoreMem(GPRClass, 2, Upper, AddrNode, GetConstant(8), 8, MEM_OFFSET_SXTX, 1);
            break;
          }
          }
        } else {
          if (Op->StoreSize != 10) { // if it's not 80bits then convert
            StackNode = IREmit->_F80CVT(Op->StoreSize, StackNode);
          }
          if (Op->StoreSize == 10) { // Part of code from StoreResult_WithOpSize()
            // For X87 extended doubles, split before storing
            IREmit->_StoreMem(FPRClass, 8, AddrNode, StackNode);
            auto Upper = IREmit->_VExtractToGPR(16, 8, StackNode, 1);
            auto DestAddr = IREmit->_Add(OpSize::i64Bit, AddrNode, GetConstant(8));
            IREmit->_StoreMem(GPRClass, 2, DestAddr, Upper, 8);
          } else {
            IREmit->_StoreMem(FPRClass, Op->StoreSize, AddrNode, StackNode);
          }
        }
        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_STORESTACKTOSTACK: { // stores top of stack in another place in stack.


        LogMan::Msg::DFmt("OP_STORESTACKTOSTACK");
        const auto* Op = IROp->C<IR::IROp_StoreStackToStack>();

        auto Offset = Op->StackLocation;

        if (Offset != 0) {
          const auto& [Valid, Value] = StackData.top();
          MigrateToSlowPathIf(Valid != StackSlot::VALID);

          // Need to store st0 to stack location - basically a copy.
          if (SlowPath) { // slow path
            LogMan::Msg::DFmt("Slow path STORESTACKTOSTACK");
            StoreStackValueAtOffset_Slow(Offset, LoadStackValueAtTop_Slow());
          } else { // fast path
            LogMan::Msg::DFmt("Fast path STORESTACKTOSTACK");
            StackData.setTop(Value, Offset);
          }
        } else {
          LogMan::Msg::DFmt("STORESTACKTOSTACK with 0 is NOP");
        }

        IREmit->Remove(CodeNode);
        break;
      }
      case IR::OP_POPSTACKDESTROY: {


        LogMan::Msg::DFmt("OP_POPSTACKDESTROY");

        if (SlowPath) { // slow path
          LogMan::Msg::DFmt("Slow path POPSTACKDESTROY");
          SetX87ValidTag(GetTopWithCache_Slow(), false);
          UpdateTop4Pop_Slow();
        } else {
          LogMan::Msg::DFmt("Fast path POPSTACKDESTROY");
          StackData.pop();
        }

        IREmit->Remove(CodeNode);
        break;
      }
      case IR::OP_F80ADDSTACK: {
        LogMan::Msg::DFmt("OP_F80ADDSTACK");
        const auto* Op = IROp->C<IR::IROp_F80AddStack>();

        // Adds two elements in the stack by offset.
        auto StackOffset1 = Op->SrcStack1;
        auto StackOffset2 = Op->SrcStack2;

        const auto& [Valid1, StackMember1] = StackData.top(StackOffset1);
        const auto& [Valid2, StackMember2] = StackData.top(StackOffset2);

        MigrateToSlowPathIf(Valid1 != StackSlot::VALID || Valid2 != StackSlot::VALID);

        if (SlowPath) { // Slow Path
          LogMan::Msg::DFmt("Slow path F80ADDSTACK");

          // Load the current value from the x87 fpu stack
          auto* StackNode1 = LoadStackValueAtOffset_Slow(StackOffset1);
          auto* StackNode2 = LoadStackValueAtOffset_Slow(StackOffset2);

          Ref AddNode = {};
          if (ReducedPrecisionMode) {
            AddNode = IREmit->_VFAdd(8, 8, StackNode1, StackNode2);
          } else {
            AddNode = IREmit->_F80Add(StackNode1, StackNode2);
          }
          StoreStackValueAtOffset_Slow(StackOffset1, AddNode, false);
        } else {
          // Fast path
          LogMan::Msg::DFmt("Fast path F80ADDSTACK");
          Ref AddNode = {};
          if (ReducedPrecisionMode) {
            AddNode = IREmit->_VFAdd(8, 8, StackMember1.StackDataNode, StackMember2.StackDataNode);
          } else {
            AddNode = IREmit->_F80Add(StackMember1.StackDataNode, StackMember2.StackDataNode);
          }
          // Store it in the stack
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember1.SourceDataSize,
                                            .StackDataSize = StackMember1.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = AddNode,
                                            .InterpretAsFloat = StackMember1.InterpretAsFloat},
                           StackOffset1);
        }

        IREmit->Remove(CodeNode);
        break;
      }
      case IR::OP_F80ADDVALUE: {


        LogMan::Msg::DFmt("F80ADDVALUE");
        const auto* Op = IROp->C<IR::IROp_F80AddValue>();
        auto* ValueNode = CurrentIR.GetNode(Op->X80Src);
        auto StackOffset = Op->SrcStack;

        const auto& [Valid, StackMember] = StackData.top(StackOffset);
        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        if (SlowPath) { // slow path
          LogMan::Msg::DFmt("Slow path F80ADDVALUE");

          // Load the current value from the x87 fpu stack
          auto* StackNode = LoadStackValueAtOffset_Slow(StackOffset);
          Ref AddNode = {};
          if (ReducedPrecisionMode) {
            AddNode = IREmit->_VFAdd(8, 8, ValueNode, StackNode);
          } else {
            AddNode = IREmit->_F80Add(ValueNode, StackNode);
          }
          // Store it in stack TOP
          LogMan::Msg::DFmt("Storing node to TOP of stack");
          // We know top is valid if it's source so no need to set it as such.
          // Only set it if source is not top.
          StoreStackValueAtTop_Slow(AddNode, StackOffset != 0);
        } else {
          LogMan::Msg::DFmt("Fast path F80ADDVALUE");

          Ref AddNode = {};
          if (ReducedPrecisionMode) {
            AddNode = IREmit->_VFAdd(8, 8, ValueNode, StackMember.StackDataNode);
          } else {
            AddNode = IREmit->_F80Add(ValueNode, StackMember.StackDataNode);
          }
          // Store it in the stack
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember.SourceDataSize,
                                            .StackDataSize = StackMember.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = AddNode,
                                            .InterpretAsFloat = StackMember.InterpretAsFloat});
        }
        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80SUBSTACK: {


        LogMan::Msg::DFmt("OP_F80SUBSTACK");
        const auto* Op = IROp->C<IR::IROp_F80SubStack>();

        // Adds two elements in the stack by offset.
        auto StackDest = Op->DstStack;
        auto StackOffset1 = Op->SrcStack1;
        auto StackOffset2 = Op->SrcStack2;

        const auto& [Valid1, StackMember1] = StackData.top(StackOffset1);
        const auto& [Valid2, StackMember2] = StackData.top(StackOffset2);

        MigrateToSlowPathIf(Valid1 != StackSlot::VALID || Valid2 != StackSlot::VALID);

        if (SlowPath) { // Slow Path
          LogMan::Msg::DFmt("Slow path F80SUBSTACK");

          // Load the current value from the x87 fpu stack
          auto* StackNode1 = LoadStackValueAtOffset_Slow(StackOffset1);
          auto* StackNode2 = LoadStackValueAtOffset_Slow(StackOffset2);

          Ref SubNode {};
          if (ReducedPrecisionMode) {
            SubNode = IREmit->_VFSub(8, 8, StackNode1, StackNode2);
          } else {
            SubNode = IREmit->_F80Sub(StackNode1, StackNode2);
          }
          StoreStackValueAtOffset_Slow(StackDest, SubNode, StackDest != StackOffset1 && StackDest != StackOffset2);
        } else {
          // Fast path
          LogMan::Msg::DFmt("Fast path F80SUBSTACK");
          Ref SubNode {};
          if (ReducedPrecisionMode) {
            SubNode = IREmit->_VFSub(8, 8, StackMember1.StackDataNode, StackMember2.StackDataNode);
          } else {
            SubNode = IREmit->_F80Sub(StackMember1.StackDataNode, StackMember2.StackDataNode);
          }
          // Store it in the stack
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember1.SourceDataSize,
                                            .StackDataSize = StackMember1.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = SubNode,
                                            .InterpretAsFloat = StackMember1.InterpretAsFloat},
                           StackDest);
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80SUBRVALUE:
      case IR::OP_F80SUBVALUE: {


        LogMan::Msg::DFmt("F80SUBVALUE");
        const auto* Op = IROp->C<IR::IROp_F80SubValue>();
        auto* ValueNode = CurrentIR.GetNode(Op->X80Src);

        auto StackOffset = Op->SrcStack;
        const auto& [Valid, StackMember] = StackData.top(StackOffset);

        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        if (SlowPath) { // slow path
          LogMan::Msg::DFmt("Slow path F80SUBVALUE");

          // Load the current value from the x87 fpu stack
          auto* StackNode = LoadStackValueAtOffset_Slow(StackOffset);

          Ref SubNode {};
          if (IROp->Op == IR::OP_F80SUBVALUE) {
            if (ReducedPrecisionMode) {
              SubNode = IREmit->_VFSub(8, 8, StackNode, ValueNode);
            } else {
              SubNode = IREmit->_F80Sub(StackNode, ValueNode);
            }
          } else {
            if (ReducedPrecisionMode) {
              SubNode = IREmit->_VFSub(8, 8, ValueNode, StackNode);
            } else {
              SubNode = IREmit->_F80Sub(ValueNode, StackNode); // IR::OP_F80SUBRVALUE
            }
          }

          // Store it in stack TOP
          StoreStackValueAtTop_Slow(SubNode, StackOffset != 0);
        } else {
          LogMan::Msg::DFmt("Fast path F80SUBVALUE");
          Ref SubNode {};
          if (IROp->Op == IR::OP_F80SUBVALUE) {
            if (ReducedPrecisionMode) {
              SubNode = IREmit->_VFSub(8, 8, StackMember.StackDataNode, ValueNode);

            } else {
              SubNode = IREmit->_F80Sub(StackMember.StackDataNode, ValueNode);
            }
          } else {
            if (ReducedPrecisionMode) {
              SubNode = IREmit->_VFSub(8, 8, ValueNode, StackMember.StackDataNode);
            } else {
              SubNode = IREmit->_F80Sub(ValueNode, StackMember.StackDataNode); // IR::OP_F80SUBRVALUE
            }
          }
          // Store it in the stack
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember.SourceDataSize,
                                            .StackDataSize = StackMember.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = SubNode,
                                            .InterpretAsFloat = StackMember.InterpretAsFloat});
        }
        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80DIVSTACK: {


        LogMan::Msg::DFmt("OP_F80DIVSTACK");
        const auto* Op = IROp->C<IR::IROp_F80DivStack>();

        // Adds two elements in the stack by offset.
        auto StackDest = Op->DstStack;
        auto StackOffset1 = Op->SrcStack1;
        auto StackOffset2 = Op->SrcStack2;

        const auto& [Valid1, StackMember1] = StackData.top(StackOffset1);
        const auto& [Valid2, StackMember2] = StackData.top(StackOffset2);

        MigrateToSlowPathIf(Valid1 != StackSlot::VALID || Valid2 != StackSlot::VALID);

        if (SlowPath) { // Slow Path

          LogMan::Msg::DFmt("Slow path F80DIVSTACK");

          // Load the current value from the x87 fpu stack
          auto* StackNode1 = LoadStackValueAtOffset_Slow(StackOffset1);
          auto* StackNode2 = LoadStackValueAtOffset_Slow(StackOffset2);

          Ref DivNode {};
          if (ReducedPrecisionMode) {
            DivNode = IREmit->_VFDiv(8, 8, StackNode1, StackNode2);
          } else {
            DivNode = IREmit->_F80Div(StackNode1, StackNode2);
          }
          StoreStackValueAtOffset_Slow(StackDest, DivNode, StackOffset1 != StackDest && StackOffset2 != StackDest);
        } else {
          // Fast path
          LogMan::Msg::DFmt("Fast path F80DIVSTACK");
          Ref DivNode {};
          if (ReducedPrecisionMode) {
            DivNode = IREmit->_VFDiv(8, 8, StackMember1.StackDataNode, StackMember2.StackDataNode);
          } else {
            DivNode = IREmit->_F80Div(StackMember1.StackDataNode, StackMember2.StackDataNode);
          }
          // Store it in the stack
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember1.SourceDataSize,
                                            .StackDataSize = StackMember1.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = DivNode,
                                            .InterpretAsFloat = StackMember1.InterpretAsFloat},
                           StackDest);
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80DIVRVALUE:
      case IR::OP_F80DIVVALUE: {


        LogMan::Msg::DFmt("F80DIVVALUE");
        const auto* Op = IROp->C<IR::IROp_F80DivValue>();
        auto* ValueNode = CurrentIR.GetNode(Op->X80Src);

        auto StackOffset = Op->SrcStack;
        const auto& [Valid, StackMember] = StackData.top(StackOffset);

        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        if (SlowPath) { // slow path
          LogMan::Msg::DFmt("Slow path F80DIVVALUE");

          // Load the current value from the x87 fpu stack
          auto* StackNode = LoadStackValueAtOffset_Slow(StackOffset);

          Ref DivNode = nullptr;
          if (IROp->Op == IR::OP_F80DIVVALUE) {
            if (ReducedPrecisionMode) {
              DivNode = IREmit->_VFDiv(8, 8, StackNode, ValueNode);
            } else {
              DivNode = IREmit->_F80Div(StackNode, ValueNode);
            }
          } else {
            if (ReducedPrecisionMode) {
              DivNode = IREmit->_VFDiv(8, 8, ValueNode, StackNode);
            } else {
              DivNode = IREmit->_F80Div(ValueNode, StackNode); // IR::OP_F80DIVRVALUE
            }
          }

          // Store it in stack TOP
          StoreStackValueAtTop_Slow(DivNode, StackOffset != 0);
        } else {
          LogMan::Msg::DFmt("Fast path F80DIVVALUE");
          Ref DivNode {};
          if (IROp->Op == IR::OP_F80DIVVALUE) {
            if (ReducedPrecisionMode) {
              DivNode = IREmit->_VFDiv(8, 8, StackMember.StackDataNode, ValueNode);
            } else {
              DivNode = IREmit->_F80Div(StackMember.StackDataNode, ValueNode);
            }
          } else {
            if (ReducedPrecisionMode) {
              DivNode = IREmit->_VFDiv(8, 8, ValueNode, StackMember.StackDataNode);
            } else {
              DivNode = IREmit->_F80Div(ValueNode, StackMember.StackDataNode); // IR::OP_F80SUBRVALUE
            }
          }
          // Store it in the stack
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember.SourceDataSize,
                                            .StackDataSize = StackMember.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = DivNode,
                                            .InterpretAsFloat = StackMember.InterpretAsFloat});
        }
        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80MULSTACK: {


        LogMan::Msg::DFmt("OP_F80MULSTACK");
        const auto* Op = IROp->C<IR::IROp_F80MulStack>();

        // Multiplies two elements in the stack by offset.
        auto StackOffset1 = Op->SrcStack1;
        auto StackOffset2 = Op->SrcStack2;

        const auto& [Valid1, StackMember1] = StackData.top(StackOffset1);
        const auto& [Valid2, StackMember2] = StackData.top(StackOffset2);

        MigrateToSlowPathIf(Valid1 != StackSlot::VALID || Valid2 != StackSlot::VALID);

        if (SlowPath) { // Slow Path
          LogMan::Msg::DFmt("Slow path F80MULSTACK");

          // Load the current value from the x87 fpu stack
          auto* StackNode1 = LoadStackValueAtOffset_Slow(StackOffset1);
          auto* StackNode2 = LoadStackValueAtOffset_Slow(StackOffset2);

          Ref MulNode = nullptr;
          if (ReducedPrecisionMode) {
            MulNode = IREmit->_VFMul(8, 8, StackNode1, StackNode2);
          } else {
            MulNode = IREmit->_F80Mul(StackNode1, StackNode2);
          }

          StoreStackValueAtOffset_Slow(StackOffset1, MulNode, false);
        } else {
          // Fast Path
          LogMan::Msg::DFmt("Fast path F80MULSTACK");
          Ref MulNode = nullptr;
          if (ReducedPrecisionMode) {
            MulNode = IREmit->_VFMul(8, 8, StackMember1.StackDataNode, StackMember2.StackDataNode);
          } else {
            MulNode = IREmit->_F80Mul(StackMember1.StackDataNode, StackMember2.StackDataNode);
          }

          // Store it in the stack
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember1.SourceDataSize,
                                            .StackDataSize = StackMember1.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = MulNode,
                                            .InterpretAsFloat = StackMember1.InterpretAsFloat},
                           StackOffset1);
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80MULVALUE: {


        LogMan::Msg::DFmt("F80MULVALUE");
        const auto* Op = IROp->C<IR::IROp_F80MulValue>();
        auto* ValueNode = CurrentIR.GetNode(Op->X80Src);

        auto StackOffset = Op->SrcStack;
        const auto& [Valid, StackMember] = StackData.top(StackOffset);

        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        if (SlowPath) { // slow path
          LogMan::Msg::DFmt("Slow path F80MulVALUE");
          // Load the current value from the x87 fpu stack
          auto* StackNode = LoadStackValueAtOffset_Slow(StackOffset);
          Ref MulNode {};
          if (ReducedPrecisionMode) {
            MulNode = IREmit->_VFMul(8, 8, ValueNode, StackNode);
          } else {
            MulNode = IREmit->_F80Mul(ValueNode, StackNode);
          }

          // Store it in stack TOP
          StoreStackValueAtTop_Slow(MulNode, StackOffset != 0);
        } else {
          LogMan::Msg::DFmt("Fast path F80MULVALUE");
          Ref MulNode {};
          if (ReducedPrecisionMode) {
            MulNode = IREmit->_VFMul(8, 8, ValueNode, StackMember.StackDataNode);
          } else {
            MulNode = IREmit->_F80Mul(ValueNode, StackMember.StackDataNode);
          }

          // Store it in the stack
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember.SourceDataSize,
                                            .StackDataSize = StackMember.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = MulNode,
                                            .InterpretAsFloat = StackMember.InterpretAsFloat});
        }
        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80STACKXCHANGE: {


        LogMan::Msg::DFmt("F80Xchange");
        const auto* Op = IROp->C<IR::IROp_F80StackXchange>();
        auto Offset = Op->SrcStack;

        const auto& [ValidTop, StackTop] = StackData.top();
        const auto& [Valid, StackMember] = StackData.top(Offset);

        MigrateToSlowPathIf(ValidTop != StackSlot::VALID || Valid != StackSlot::VALID);

        if (SlowPath) { // slow path
          auto* ValueTop = LoadStackValueAtTop_Slow();
          auto* ValueOffset = LoadStackValueAtOffset_Slow(Offset);
          StoreStackValueAtTop_Slow(ValueOffset, false);
          StoreStackValueAtOffset_Slow(Offset, ValueTop, false);
        } else { // fast path
          auto Tmp = StackTop;
          StackData.setTop(StackMember);
          StackData.setTop(Tmp, Offset);
        }
        IREmit->Remove(CodeNode);
        break;
      }

      case OP_F80STACKCHANGESIGN: {


        LogMan::Msg::DFmt("F80ChangeSign");
        const auto& [Valid, StackMember] = StackData.top();

        // We need a couple of intermediate instructions to change the sign
        // of a value
        Ref HelperNode {};
        if (!ReducedPrecisionMode) {
          Ref Low = GetConstant(0);
          Ref High = GetConstant(0b1'000'0000'0000'0000ULL);
          HelperNode = IREmit->_VCastFromGPR(16, 8, Low);
          HelperNode = IREmit->_VInsGPR(16, 8, 1, HelperNode, High);
        }
        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        if (SlowPath) { // slow path
          Ref ResultNode {};
          Ref Value = LoadStackValueAtTop_Slow();
          // Negate value
          if (ReducedPrecisionMode) {
            ResultNode = IREmit->_VFNeg(8, 8, Value);
          } else {
            ResultNode = IREmit->_VXor(16, 1, Value, HelperNode);
          }
          StoreStackValueAtTop_Slow(ResultNode, false);
        } else { // fast path
          StackData.setTop(StackMemberInfo {
            .SourceDataSize = StackMember.SourceDataSize,
            .StackDataSize = StackMember.StackDataSize,
            .SourceDataNode = nullptr,
            .StackDataNode = ReducedPrecisionMode ? IREmit->_VFNeg(8, 8, StackMember.StackDataNode) :
                                                    IREmit->_VXor(16, 1, StackMember.StackDataNode, HelperNode),

          });
        }
        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80STACKABS: {


        LogMan::Msg::DFmt("F80Abs");
        const auto& [Valid, StackMember] = StackData.top();

        Ref HelperNode {};
        if (!ReducedPrecisionMode) {
          // Intermediate insts
          // FIXME: maybe we could just remove this OP and have a VAND Stack instead.
          Ref Low = GetConstant(~0ULL);
          Ref High = GetConstant(0b0'111'1111'1111'1111ULL);
          HelperNode = IREmit->_VCastFromGPR(16, 8, Low);
          HelperNode = IREmit->_VInsGPR(16, 8, 1, HelperNode, High);
        }

        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        if (SlowPath) {
          // slow path
          Ref Value = LoadStackValueAtTop_Slow();
          Ref ResultNode {};
          if (ReducedPrecisionMode) {
            ResultNode = IREmit->_VFAbs(8, 8, Value);
          } else {
            ResultNode = IREmit->_VAnd(16, 1, Value, HelperNode);
          }
          StoreStackValueAtTop_Slow(ResultNode, false);
        } else {
          // fast path
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember.SourceDataSize,
                                            .StackDataSize = StackMember.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = ReducedPrecisionMode ? IREmit->_VFAbs(8, 8, StackMember.StackDataNode) :
                                                                                    IREmit->_VAnd(16, 1, StackMember.StackDataNode, HelperNode),
                                            .InterpretAsFloat = StackMember.InterpretAsFloat});
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80STACKFYL2X: {


        LogMan::Msg::DFmt("OP_F80STACKFYL2X");
        const auto& [Valid1, StackMember1] = StackData.top();
        const auto& [Valid2, StackMember2] = StackData.top(1);

        MigrateToSlowPathIf(Valid1 != StackSlot::VALID || Valid2 != StackSlot::VALID);

        if (SlowPath) {
          // slow path
          auto* st0 = LoadStackValueAtTop_Slow();
          auto* st1 = LoadStackValueAtOffset_Slow(1);

          Ref Result {};
          if (ReducedPrecisionMode) {
            Result = IREmit->_F64FYL2X(st0, st1);
          } else {
            Result = IREmit->_F80FYL2X(st0, st1);
          }
          StoreStackValueAtOffset_Slow(1, Result, false); // stores at st1
          UpdateTop4Pop_Slow();                           // updates top
        } else {
          // fast path
          auto Tmp = StackMember1;
          StackData.pop(); // we need to write the result st1, so if popping and setTop has the same behaviour
          StackData.setTop(
            StackMemberInfo {.SourceDataSize = StackMember1.SourceDataSize,
                             .StackDataSize = StackMember1.StackDataSize,
                             .SourceDataNode = nullptr,
                             .StackDataNode = ReducedPrecisionMode ? IREmit->_F64FYL2X(Tmp.StackDataNode, StackMember2.StackDataNode) :
                                                                     IREmit->_F80FYL2X(Tmp.StackDataNode, StackMember2.StackDataNode),
                             .InterpretAsFloat = StackMember1.InterpretAsFloat});
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80CMPSTACK: {


        LogMan::Msg::DFmt("OP_F80CMPSTACK");
        const auto* Op = IROp->C<IR::IROp_F80CmpStack>();
        auto offset = Op->SrcStack;
        const auto& [Valid1, StackMember1] = StackData.top();
        const auto& [Valid2, StackMember2] = StackData.top(offset);

        MigrateToSlowPathIf(Valid1 != StackSlot::VALID || Valid2 != StackSlot::VALID);

        Ref CmpNode {};
        if (SlowPath) {
          // slow path
          LogMan::Msg::DFmt("OP_F80CMPSTACK Slow");
          Ref StackValue1 = LoadStackValueAtTop_Slow();
          Ref StackValue2 = LoadStackValueAtOffset_Slow(offset);
          if (ReducedPrecisionMode) {
            CmpNode = IREmit->_FCmp(8, StackValue1, StackValue2);
          } else {
            CmpNode = IREmit->_F80Cmp(StackValue1, StackValue2, Op->Flags);
          }
        } else {
          // fast path
          LogMan::Msg::DFmt("OP_F80CMPSTACK Fast");
          if (ReducedPrecisionMode) {
            CmpNode = IREmit->_FCmp(8, StackMember1.StackDataNode, StackMember2.StackDataNode);
          } else {
            CmpNode = IREmit->_F80Cmp(StackMember1.StackDataNode, StackMember2.StackDataNode, Op->Flags);
          }
        }
        IREmit->ReplaceUsesWithAfter(CodeNode, CmpNode, CodeNode);
        IREmit->Remove(CodeNode);
        break;
      }
      case IR::OP_F80STACKTEST: {


        LogMan::Msg::DFmt("OP_F80STACKTEST");
        const auto* Op = IROp->C<IR::IROp_F80StackTest>();
        auto offset = Op->SrcStack;
        const auto& [Valid, StackMember] = StackData.top(offset);

        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        Ref CmpNode {};
        Ref ZeroConst = IREmit->_VCastFromGPR(ReducedPrecisionMode ? 8 : 16, 8, GetConstant(0));

        if (SlowPath) {
          // slow path
          LogMan::Msg::DFmt("OP_F80STACKTEST Slow");
          auto* StackValue = LoadStackValueAtOffset_Slow(offset);
          if (ReducedPrecisionMode) {
            CmpNode = IREmit->_FCmp(8, StackValue, ZeroConst);
          } else {
            CmpNode = IREmit->_F80Cmp(StackValue, ZeroConst, Op->Flags);
          }
        } else {
          // fast path
          LogMan::Msg::DFmt("OP_F80STACKTEST Fast");
          if (ReducedPrecisionMode) {
            CmpNode = IREmit->_FCmp(8, StackMember.StackDataNode, ZeroConst);
          } else {
            CmpNode = IREmit->_F80Cmp(StackMember.StackDataNode, ZeroConst, Op->Flags);
          }
        }
        IREmit->ReplaceUsesWithAfter(CodeNode, CmpNode, CodeNode);
        IREmit->Remove(CodeNode);
        break;
      }


      case IR::OP_F80CMPVALUE: {


        LogMan::Msg::DFmt("OP_F80CMPVALUE");
        const auto* Op = IROp->C<IR::IROp_F80CmpValue>();
        const auto& Value = CurrentIR.GetNode(Op->X80Src);
        const auto& [Valid, StackMember] = StackData.top();

        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        Ref CmpNode = nullptr;
        if (SlowPath) {
          // slow path
          auto* StackValue = LoadStackValueAtTop_Slow();
          if (ReducedPrecisionMode) {
            CmpNode = IREmit->_FCmp(8, StackValue, Value);
          } else {
            CmpNode = IREmit->_F80Cmp(StackValue, Value, Op->Flags);
          }
        } else {
          // fast path
          if (ReducedPrecisionMode) {
            CmpNode = IREmit->_FCmp(8, StackMember.StackDataNode, Value);
          } else {
            CmpNode = IREmit->_F80Cmp(StackMember.StackDataNode, Value, Op->Flags);
          }
        }

        IREmit->Remove(CodeNode);
        IREmit->ReplaceUsesWithAfter(CodeNode, CmpNode, CodeNode);
        break;
      }

      case IR::OP_F80ATANSTACK: {


        LogMan::Msg::DFmt("OP_F80ATANSTACK");
        const auto& [Valid1, StackMember1] = StackData.top();
        const auto& [Valid2, StackMember2] = StackData.top(1);

        MigrateToSlowPathIf(Valid1 != StackSlot::VALID || Valid2 != StackSlot::VALID);

        if (SlowPath) {
          // slow path
          Ref st0 = LoadStackValueAtTop_Slow();
          Ref st1 = LoadStackValueAtOffset_Slow(1);
          Ref Result {};
          if (ReducedPrecisionMode) {
            Result = IREmit->_F64ATAN(st1, st0);
          } else {
            Result = IREmit->_F80ATAN(st1, st0);
          }
          StoreStackValueAtOffset_Slow(1, Result, false);
          UpdateTop4Pop_Slow();
        } else {
          // fast path
          Ref st0 = StackMember1.StackDataNode;
          Ref st1 = StackMember2.StackDataNode;

          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember1.SourceDataSize,
                                            .StackDataSize = StackMember1.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = ReducedPrecisionMode ? IREmit->_F64ATAN(st1, st0) : IREmit->_F80ATAN(st1, st0),
                                            .InterpretAsFloat = StackMember1.InterpretAsFloat},
                           1);
          StackData.pop();
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80PTANSTACK: {


        LogMan::Msg::DFmt("OP_F80PTANSTACK");
        const auto& [Valid, StackMember] = StackData.top();

        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        Ref OneConst {};
        if (ReducedPrecisionMode) {
          OneConst = IREmit->_VCastFromGPR(8, 8, GetConstant(0x3FF0000000000000));
        } else {
          OneConst = IREmit->_LoadNamedVectorConstant(16, NamedVectorConstant::NAMED_VECTOR_X87_ONE);
        }

        if (SlowPath) {
          // slow path
          auto* st0 = LoadStackValueAtTop_Slow();
          Ref Value = {};
          if (ReducedPrecisionMode) {
            Value = IREmit->_F64TAN(st0);
          } else {
            Value = IREmit->_F80TAN(st0);
          }
          StoreStackValueAtTop_Slow(Value, false);
          UpdateTop4Push_Slow();
          StoreStackValueAtTop_Slow(OneConst);
        } else {
          // fast path
          auto* st0 = StackMember.StackDataNode;
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember.SourceDataSize,
                                            .StackDataSize = StackMember.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = ReducedPrecisionMode ? IREmit->_F64TAN(st0) : IREmit->_F80TAN(st0),
                                            .InterpretAsFloat = true});
          StackData.push(StackMemberInfo {.SourceDataSize = StackMember.SourceDataSize,
                                          .StackDataSize = StackMember.StackDataSize,
                                          .SourceDataNode = nullptr,
                                          .StackDataNode = OneConst,
                                          .InterpretAsFloat = false});
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80XTRACTSTACK: {


        LogMan::Msg::DFmt("OP_F80XTRACTSTACK");
        const auto& [Valid, StackMember] = StackData.top();

        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        if (SlowPath) {
          // slow path
          Ref st0 = LoadStackValueAtTop_Slow();
          Ref Exp {};
          Ref Sig {};
          if (ReducedPrecisionMode) {
            std::tie(Exp, Sig) = SplitF64SigExp(st0);
          } else {
            Exp = IREmit->_F80XTRACT_EXP(st0);
            Sig = IREmit->_F80XTRACT_SIG(st0);
          }
          // Write exp to top, update top for a push and set sig at new top.
          StoreStackValueAtTop_Slow(Exp, false);
          UpdateTop4Push_Slow();
          StoreStackValueAtTop_Slow(Sig);
        } else {
          // fast path
          Ref st0 = StackMember.StackDataNode;
          Ref Exp {};
          Ref Sig {};

          if (ReducedPrecisionMode) {
            std::tie(Exp, Sig) = SplitF64SigExp(st0);
          } else {
            Exp = IREmit->_F80XTRACT_EXP(st0);
            Sig = IREmit->_F80XTRACT_SIG(st0);
          }
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember.SourceDataSize,
                                            .StackDataSize = StackMember.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = Exp,
                                            .InterpretAsFloat = StackMember.InterpretAsFloat});
          StackData.push(StackMemberInfo {.SourceDataSize = StackMember.SourceDataSize,
                                          .StackDataSize = StackMember.StackDataSize,
                                          .SourceDataNode = nullptr,
                                          .StackDataNode = Sig,
                                          .InterpretAsFloat = StackMember.InterpretAsFloat});
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80FPREMSTACK: {


        LogMan::Msg::DFmt("F80FPREMStack");
        const auto& [Valid1, StackMember1] = StackData.top();
        const auto& [Valid2, StackMember2] = StackData.top(1);

        MigrateToSlowPathIf(Valid1 != StackSlot::VALID || Valid2 != StackSlot::VALID);

        if (SlowPath) {
          // slow path
          auto* st0 = LoadStackValueAtTop_Slow();
          auto* st1 = LoadStackValueAtOffset_Slow(1);

          Ref Value {};
          if (ReducedPrecisionMode) {
            Value = IREmit->_F64FPREM(st0, st1);
          } else {
            Value = IREmit->_F80FPREM(st0, st1);
          }
          StoreStackValueAtTop_Slow(Value, false);
        } else { // fast path
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember1.SourceDataSize,
                                            .StackDataSize = StackMember1.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = ReducedPrecisionMode ?
                                                               IREmit->_F64FPREM(StackMember1.StackDataNode, StackMember2.StackDataNode) :
                                                               IREmit->_F80FPREM(StackMember1.StackDataNode, StackMember2.StackDataNode),
                                            .InterpretAsFloat = StackMember1.InterpretAsFloat});
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80FPREM1STACK: {


        LogMan::Msg::DFmt("F80FPREM1Stack");
        const auto& [Valid1, StackMember1] = StackData.top();
        const auto& [Valid2, StackMember2] = StackData.top(1);

        MigrateToSlowPathIf(Valid1 != StackSlot::VALID || Valid2 != StackSlot::VALID);

        if (SlowPath) {
          // slow path
          auto* st0 = LoadStackValueAtTop_Slow();
          auto* st1 = LoadStackValueAtOffset_Slow(1);

          Ref Value {};
          if (ReducedPrecisionMode) {
            Value = IREmit->_F64FPREM1(st0, st1);
          } else {
            Value = IREmit->_F80FPREM1(st0, st1);
          }
          StoreStackValueAtTop_Slow(Value, false);
        } else { // fast path
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember1.SourceDataSize,
                                            .StackDataSize = StackMember1.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = ReducedPrecisionMode ?
                                                               IREmit->_F64FPREM1(StackMember1.StackDataNode, StackMember2.StackDataNode) :
                                                               IREmit->_F80FPREM1(StackMember1.StackDataNode, StackMember2.StackDataNode),
                                            .InterpretAsFloat = StackMember1.InterpretAsFloat});
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80SCALESTACK: {


        LogMan::Msg::DFmt("F80FPSCALE");
        const auto& [Valid1, StackMember1] = StackData.top();
        const auto& [Valid2, StackMember2] = StackData.top(1);

        MigrateToSlowPathIf(Valid1 != StackSlot::VALID || Valid2 != StackSlot::VALID);

        if (SlowPath) {
          // slow path
          auto* st0 = LoadStackValueAtTop_Slow();
          auto* st1 = LoadStackValueAtOffset_Slow(1);

          Ref Value {};
          if (ReducedPrecisionMode) {
            Value = IREmit->_F64SCALE(st0, st1);
          } else {
            Value = IREmit->_F80SCALE(st0, st1);
          }
          StoreStackValueAtTop_Slow(Value, false);
        } else { // fast path
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember1.SourceDataSize,
                                            .StackDataSize = StackMember1.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = ReducedPrecisionMode ?
                                                               IREmit->_F64SCALE(StackMember1.StackDataNode, StackMember2.StackDataNode) :
                                                               IREmit->_F80SCALE(StackMember1.StackDataNode, StackMember2.StackDataNode),
                                            .InterpretAsFloat = StackMember1.InterpretAsFloat});
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80SQRTSTACK: {


        LogMan::Msg::DFmt("F80SQRTSTACK");
        const auto& [Valid, StackMember] = StackData.top();

        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        if (SlowPath) {
          // slow path
          auto* st0 = LoadStackValueAtTop_Slow();
          Ref Value {};
          if (ReducedPrecisionMode) {
            Value = IREmit->_VFSqrt(8, 8, st0);
          } else {
            Value = IREmit->_F80SQRT(st0);
          }
          StoreStackValueAtTop_Slow(Value, false);
        } else {
          // fast path
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember.SourceDataSize,
                                            .StackDataSize = StackMember.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = ReducedPrecisionMode ? IREmit->_VFSqrt(8, 8, StackMember.StackDataNode) :
                                                                                    IREmit->_F80SQRT(StackMember.StackDataNode),
                                            .InterpretAsFloat = StackMember.InterpretAsFloat});
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80SINSTACK: {


        LogMan::Msg::DFmt("F80SINSTACK");
        const auto& [Valid, StackMember] = StackData.top();

        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        if (SlowPath) {
          // slow path
          auto* st0 = LoadStackValueAtTop_Slow();
          Ref Value {};
          if (ReducedPrecisionMode) {
            Value = IREmit->_F64SIN(st0);
          } else {
            Value = IREmit->_F80SIN(st0);
          }
          StoreStackValueAtTop_Slow(Value, false);
        } else {
          // fast path
          StackData.setTop(StackMemberInfo {
            .SourceDataSize = StackMember.SourceDataSize,
            .StackDataSize = StackMember.StackDataSize,
            .SourceDataNode = nullptr,
            .StackDataNode = ReducedPrecisionMode ? IREmit->_F64SIN(StackMember.StackDataNode) : IREmit->_F80SIN(StackMember.StackDataNode),
            .InterpretAsFloat = StackMember.InterpretAsFloat});
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80COSSTACK: {


        LogMan::Msg::DFmt("F80COSSTACK");
        const auto& [Valid, StackMember] = StackData.top();

        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        if (SlowPath) {
          // slow path
          auto* st0 = LoadStackValueAtTop_Slow();
          Ref Value {};
          if (ReducedPrecisionMode) {
            Value = IREmit->_F64COS(st0);
          } else {
            Value = IREmit->_F80COS(st0);
          }
          StoreStackValueAtTop_Slow(Value, false);
        } else {
          // fast path
          StackData.setTop(StackMemberInfo {
            .SourceDataSize = StackMember.SourceDataSize,
            .StackDataSize = StackMember.StackDataSize,
            .SourceDataNode = nullptr,
            .StackDataNode = ReducedPrecisionMode ? IREmit->_F64COS(StackMember.StackDataNode) : IREmit->_F80COS(StackMember.StackDataNode),
            .InterpretAsFloat = StackMember.InterpretAsFloat});
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80F2XM1STACK: {


        LogMan::Msg::DFmt("F80F2XM1STACK");
        const auto& [Valid, StackMember] = StackData.top();

        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        if (SlowPath) {
          // slow path
          auto* st0 = LoadStackValueAtTop_Slow();
          Ref Value {};
          if (ReducedPrecisionMode) {
            Value = IREmit->_F64F2XM1(st0);
          } else {
            Value = IREmit->_F80F2XM1(st0);
          }
          StoreStackValueAtTop_Slow(Value, false);
        } else {
          // fast path
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember.SourceDataSize,
                                            .StackDataSize = StackMember.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = ReducedPrecisionMode ? IREmit->_F64F2XM1(StackMember.StackDataNode) :
                                                                                    IREmit->_F80F2XM1(StackMember.StackDataNode),
                                            .InterpretAsFloat = StackMember.InterpretAsFloat});
        }

        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_SYNCSTACKTOSLOW: {


        LogMan::Msg::DFmt("SYNCSTACKTOSLOW");

        // This synchronizes stack values but doesn't necessarily moves us off the FastPath!
        Ref NewTop = SynchronizeStackValues();
        IREmit->ReplaceUsesWithAfter(CodeNode, NewTop, CodeNode);
        IREmit->Remove(CodeNode);

        break;
      }

      case IR::OP_STACKFORCESLOW: {


        LogMan::Msg::DFmt("STACKFORCESLOW");

        MigrateToSlowPathIf(true);
        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_INCSTACKTOP: {


        LogMan::Msg::DFmt("INCSTACKTOP");

        if (SlowPath) {
          UpdateTop4Pop_Slow();
        } else {
          StackData.rotate(false);
        }
        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_DECSTACKTOP: {


        LogMan::Msg::DFmt("DECSTACKTOP");

        if (SlowPath) {
          UpdateTop4Push_Slow();
        } else {
          StackData.rotate(true);
        }
        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80ROUNDSTACK: {


        LogMan::Msg::DFmt("F80ROUNDSTACK");

        const auto& [Valid, StackMember] = StackData.top();
        MigrateToSlowPathIf(Valid != StackSlot::VALID);

        if (SlowPath) {
          auto* st0 = LoadStackValueAtTop_Slow();
          Ref Value {};
          if (ReducedPrecisionMode) {
            Value = IREmit->_Vector_FToI(8, 8, st0, FEXCore::IR::Round_Host);
          } else {
            Value = IREmit->_F80Round(st0);
          }
          StoreStackValueAtTop_Slow(Value, false);
        } else {
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember.SourceDataSize,
                                            .StackDataSize = StackMember.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = ReducedPrecisionMode ?
                                                               IREmit->_Vector_FToI(8, 8, StackMember.StackDataNode, FEXCore::IR::Round_Host) :
                                                               IREmit->_F80Round(StackMember.StackDataNode),
                                            .InterpretAsFloat = false});
        }
        IREmit->Remove(CodeNode);
        break;
      }

      case IR::OP_F80VBSLSTACK: {


        LogMan::Msg::DFmt("F80VBSLStack");

        const auto* Op = IROp->C<IR::IROp_F80VBSLStack>();

        // Multiplies two elements in the stack by offset.
        auto* VecCond = CurrentIR.GetNode(Op->VectorMask);
        auto StackOffset1 = Op->SrcStack1;
        auto StackOffset2 = Op->SrcStack2;

        const auto& [Valid1, StackMember1] = StackData.top(StackOffset1);
        const auto& [Valid2, StackMember2] = StackData.top(StackOffset2);

        MigrateToSlowPathIf(Valid1 != StackSlot::VALID || Valid2 != StackSlot::VALID);

        if (SlowPath) { // Slow Path
          auto* Value1 = LoadStackValueAtOffset_Slow(StackOffset1);
          auto* Value2 = LoadStackValueAtOffset_Slow(StackOffset2);

          StoreStackValueAtTop_Slow(IREmit->_VBSL(16, VecCond, Value1, Value2), StackOffset1 != 0 && StackOffset2 != 0);
        } else {
          StackData.setTop(StackMemberInfo {.SourceDataSize = StackMember1.SourceDataSize,
                                            .StackDataSize = StackMember1.StackDataSize,
                                            .SourceDataNode = nullptr,
                                            .StackDataNode = IREmit->_VBSL(16, VecCond, StackMember1.StackDataNode, StackMember2.StackDataNode),
                                            .InterpretAsFloat = false});
        }

        IREmit->Remove(CodeNode);
        break;
      }

      default: break;
      }
    }
    LogMan::Msg::DFmt("Finished Block Processing");

    // We need to write the registers before any branch in the block,
    // so loop until a branch instruction is found. Add instructions _before_
    // the branch instruction.
    [[maybe_unused]] bool ExitFound = false; // used for assertion
    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      if (IR::IsBlockExit(IROp->Op)) {
        LogMan::Msg::DFmt("Found a block exit!");
        // Set write cursor to previous instruction
        IREmit->SetWriteCursor(IREmit->UnwrapNode(CodeNode->Header.Previous));
        ExitFound = true;
        break;
      }
    }
    LOGMAN_THROW_A_FMT(ExitFound, "No block exit found is bad!");
    SynchronizeStackValues();
    IREmit->SetWriteCursor(OriginalWriteCursor);
    LogMan::Msg::DFmt("===================================================\n\n");
  }

  return;
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateX87StackOptimizationPass() {
  return fextl::make_unique<X87StackOptimization>();
}
} // namespace FEXCore::IR
