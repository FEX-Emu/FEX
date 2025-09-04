// SPDX-License-Identifier: MIT
#include "FEXCore/Utils/LogManager.h"
#include "Interface/Core/Interpreter/Fallbacks/FallbackOpHandler.h"
#include "Interface/IR/IR.h"
#include "Interface/IR/IREmitter.h"
#include "Interface/IR/PassManager.h"
#include "FEXCore/IR/IR.h"
#include "FEXCore/Utils/Profiler.h"
#include "FEXCore/Utils/MathUtils.h"
#include "FEXCore/Core/HostFeatures.h"
#include "Interface/Core/Addressing.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdint.h>

// This file adds a pass to process X87 stack instructions.
// These instructions are marked in IR.json with `X87: true` and are generated
// by X87 guest instructions.
// The way is works is that there's a virtual stack `StackData`, where we load and store
// and apply the operations in a block of code. Once the block finishes, we emit the necessary operations
// that we recorded onto the virtual stack. This allows us to save a lot of code movement
// to and from stack registers, top management and valid flags. It also allows us to
// perform memcpy optimizations like the one performed in STORESTACKMEM.
//
// By default we run on the fast path - i.e. we assume all values are in the stack and we have a complete
// stack overview. However, if we encounter a value that's not in the virtual stack - maybe it was added
// to the stack in a previous block, we move onto the slow path which loads and stores values to the stack
// registers.
// Once in a slow path, we won't return to the fast pass until the beginning of the following block.

namespace FEXCore::IR {

// FIXME(pmatos): copy from OpcodeDispatcher.h
inline uint32_t MMBaseOffset() {
  return static_cast<uint32_t>(offsetof(Core::CPUState, mm[0][0]));
}

// Similar helper to the one in OpcodeDispatcher.h except we do not
// need to handle flags, etc.
template<typename T>
void DeriveOp(Ref& RefV, IROps NewOp, IREmitter::IRPair<T> Expr) {
  Expr.first->Header.Op = NewOp;
  RefV = Expr;
}

enum class StackSlot { UNUSED, INVALID, VALID };
// FixedSizeStack is a model of the x87 Stack where each element in this
// fixed size stack lives at an offset from top. The top of the stack is at
// index 0.
template<typename T>
class FixedSizeStack {
public:
  struct StackSlotEntry final {
    StackSlot Type;
    T Value;
  };

  static constexpr uint8_t size = 8;

  // Real top as an offset from stored top value (or the one at the beginning of the block)
  // For example, if we start and push a value to our simulated stack, because we don't
  // update top straight away the TopOffset is 1.
  // If SlowPath is true, then TopOffset is always zero.
  int8_t TopOffset = 0;

  FixedSizeStack()
    : buffer(FixedSizeStack::size, {StackSlot::UNUSED, T()}) {}

  void push(const T& Value) {
    rotate();
    buffer.front() = {StackSlot::VALID, Value};
  }

  // Rotate the elements with the direction controlled by Right
  void rotate(bool Right = true) {
    if (Right) {
      std::rotate(buffer.begin(), buffer.end() - 1, buffer.end());
      TopOffset++;
    } else {
      std::rotate(buffer.begin(), buffer.begin() + 1, buffer.end());
      TopOffset--;
    }
  }

  void pop() {
    buffer.front() = {StackSlot::INVALID, T()};
    rotate(false);
  }

  const StackSlotEntry& top(size_t Offset = 0) const {
    return buffer[Offset];
  }

  void setTop(T Value, size_t Offset = 0) {
    buffer[Offset] = {StackSlot::VALID, Value};
  }

  bool isValid(size_t Offset) const {
    return buffer[Offset].first;
  }

  void clear() {
    for (auto& Elem : buffer) {
      Elem = {StackSlot::UNUSED, T()};
    }
    TopOffset = 0;
  }

  void dump() const {
    LogMan::Msg::DFmt("-- Stack");

    for (size_t i = 0; i < 8; i++) {
      const auto& [Valid, Element] = buffer[i];
      if (Valid == StackSlot::VALID) {
        LogMan::Msg::DFmt("| ST{}: 0x{:x}", i, (uintptr_t)(Element.StackDataNode));
      } else if (Valid == StackSlot::INVALID) {
        LogMan::Msg::DFmt("| ST{}: INVALID", i);
      }
    }
    LogMan::Msg::DFmt("--");
  }

  void setTagInvalid(size_t Index) {
    buffer[Index].Type = StackSlot::INVALID;
  }

  // Returns a mask to set in AbridgedTagWord
  uint8_t getValidMask() {
    uint8_t Mask = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
      if (buffer[i].Type == StackSlot::VALID) {
        Mask |= 1U << i;
      }
    }
    return Mask;
  }

  // Returns a mask to set in AbridgedTagWord
  uint8_t getInvalidMask() {
    uint8_t Mask = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
      if (buffer[i].Type == StackSlot::INVALID) {
        Mask |= 1U << i;
      }
    }
    return Mask;
  }

private:
  fextl::vector<StackSlotEntry> buffer;
};

class X87StackOptimization final : public Pass {
public:
  X87StackOptimization(const FEXCore::HostFeatures& Features, OpSize GPROpSize)
    : Features(Features)
    , GPROpSize(GPROpSize) {
    FEX_CONFIG_OPT(ReducedPrecision, X87REDUCEDPRECISION);
    ReducedPrecisionMode = ReducedPrecision;
  }
  void Run(IREmitter* Emit) override;

private:
  const FEXCore::HostFeatures& Features;
  const OpSize GPROpSize;
  bool ReducedPrecisionMode;
  FEX_CONFIG_OPT(DisableVixlIndirectCalls, DISABLE_VIXL_INDIRECT_RUNTIME_CALLS);

  // Helpers
  Ref RotateRight8(uint32_t V, Ref Amount);

  void F80SplitStore_Helper(const IROp_StoreStackMem* Op, Ref StackNode) {
    Ref AddrNode = IR->GetNode(Op->Addr);
    Ref Offset = IR->GetNode(Op->Offset);
    OpSize Align = Op->Align;
    MemOffsetType OffsetType = Op->OffsetType;
    uint8_t OffsetScale = Op->OffsetScale;

    IREmit->_StoreMem(FPRClass, OpSize::i64Bit, StackNode, AddrNode, Offset, Align, OffsetType, OffsetScale);
    auto Upper = IREmit->_VExtractToGPR(OpSize::i128Bit, OpSize::i64Bit, StackNode, 1);

    // Store the Upper part of the register (the remaining 2 bytes) into memory.
    AddressMode A {.Base = AddrNode,
                   .Index = Op->Offset.IsInvalid() ? nullptr : Offset,
                   .IndexType = MEM_OFFSET_SXTX,
                   .IndexScale = OffsetScale,
                   .Offset = 8,
                   .AddrSize = OpSize::i64Bit};
    A = SelectAddressMode(IREmit, A, GPROpSize, Features.SupportsTSOImm9, false, false, OpSize::i16Bit);
    IREmit->_StoreMem(GPRClass, OpSize::i16Bit, Upper, A.Base, A.Index, OpSize::i64Bit, MEM_OFFSET_SXTX, A.IndexScale);
  }

  void StoreStackMem_Helper(const IROp_StoreStackMem* Op, Ref StackNode) {
    Ref AddrNode = IR->GetNode(Op->Addr);
    Ref Offset = IR->GetNode(Op->Offset);
    OpSize Align = Op->Align;
    MemOffsetType OffsetType = Op->OffsetType;
    uint8_t OffsetScale = Op->OffsetScale;

    // Normal Precision Mode
    switch (Op->StoreSize) {
    case OpSize::i32Bit:
    case OpSize::i64Bit: {
      StackNode = IREmit->_F80CVT(Op->StoreSize, StackNode);
      IREmit->_StoreMem(FPRClass, Op->StoreSize, StackNode, AddrNode, Offset, Align, OffsetType, OffsetScale);
      break;
    }

    case OpSize::f80Bit: {
      if (Features.SupportsSVE128 || Features.SupportsSVE256) {
        AddressMode A {.Base = AddrNode,
                       .Index = Op->Offset.IsInvalid() ? nullptr : Offset,
                       .IndexType = MEM_OFFSET_SXTX,
                       .IndexScale = OffsetScale,
                       .AddrSize = OpSize::i64Bit};
        AddrNode = LoadEffectiveAddress(IREmit, A, GPROpSize, false);
        IREmit->_StoreMemX87SVEOptPredicate(OpSize::i128Bit, OpSize::i16Bit, StackNode, AddrNode);
      } else { // 80bit requires split-store
        F80SplitStore_Helper(Op, StackNode);
      }
      break;
    }
    default: ERROR_AND_DIE_FMT("Unsupported x87 size");
    }
  }

  // Performs a store to memory from a value the stack passed in as StackNode.
  // This is the version dealing with the reduced precision case.
  void StoreStackMem_Reduced_Helper(const IROp_StoreStackMem* Op, Ref StackNode) {
    Ref AddrNode = IR->GetNode(Op->Addr);
    Ref Offset = IR->GetNode(Op->Offset);
    OpSize Align = Op->Align;
    MemOffsetType OffsetType = Op->OffsetType;
    uint8_t OffsetScale = Op->OffsetScale;

    switch (Op->StoreSize) {
    case OpSize::i32Bit: {
      StackNode = IREmit->_Float_FToF(OpSize::i32Bit, OpSize::i64Bit, StackNode);
      [[fallthrough]];
    }
    case OpSize::i64Bit: {
      IREmit->_StoreMem(FPRClass, Op->StoreSize, StackNode, AddrNode, Offset, Align, OffsetType, OffsetScale);
      break;
    }

    // 80bit requires split-store
    case OpSize::f80Bit: {
      StackNode = IREmit->_F80CVTTo(StackNode, OpSize::i64Bit);
      F80SplitStore_Helper(Op, StackNode);
      break;
    }
    default: ERROR_AND_DIE_FMT("Unsupported x87 size");
    }
  }


  // Helper to check if a Ref is a Zero constant
  bool IsZero(Ref Node) {
    auto Header = IR->GetOp<IR::IROp_Header>(Node);
    if (Header->Op != OP_CONSTANT) {
      return false;
    }

    auto Const = Header->C<IROp_Constant>();
    return Const->Constant == 0;
  }

  // Handles a Unary operation.
  // Takes the op we are handling, the Node for the reduced precision case and the node for the normal case.
  // Depending on the type of Op64, we might need to pass a couple of extra constant arguments, this happens
  // when VFOp64 is true.
  void HandleUnop(IROps Op64, bool VFOp64, IROps Op80);
  void HandleBinopValue(IROps Op64, bool VFOp64, IROps Op80, uint8_t DestStackOffset, bool MarkDestValid, uint8_t StackOffset,
                        Ref ValueNode, bool Reverse = false);
  void HandleBinopStack(IROps Op64, bool VFOp64, IROps Op80, uint8_t DestStackOffset, uint8_t StackOffset1, uint8_t StackOffset2,
                        bool Reverse = false);

  // Top Management Helpers
  /// Set the valid tag for Value as valid (if Valid is true), or invalid (if Valid is false).
  void SetX87ValidTag(Ref Value, bool Valid);
  // Generates slow code to load/store a value from an offset from the top of the stack
  Ref LoadStackValueAtOffset_Slow(uint8_t Offset = 0);
  void StoreStackValueAtOffset_Slow(Ref Value, uint8_t Offset = 0, bool SetValid = true);
  // Update Top value in slow path for a pop
  void UpdateTopForPop_Slow();
  void UpdateTopForPush_Slow();
  // Synchronizes the current simulated stack with the actual values.
  // Returns a new value for Top, that's synchronized between the simulated stack
  // and the actual FPU stack.
  Ref SynchronizeStackValues();
  // Moves us from the fast to the slow path if ShouldMigrate is true.
  void MigrateToSlowPathIf(bool ShouldMigrate);
  // Top Cache Management
  Ref GetTopWithCache_Slow();
  Ref GetOffsetTopWithCache_Slow(uint8_t Offset, bool Reverse = false);
  void SetTopWithCache_Slow(Ref Value);
  Ref GetX87ValidTag_Slow(uint8_t Offset);
  // Resets fields to initial values
  void Reset();

  struct StackMemberInfo {
    StackMemberInfo() {}
    StackMemberInfo(Ref Data)
      : StackDataNode(Data) {}
    StackMemberInfo(Ref Data, Ref Source, OpSize Size, bool Float)
      : StackDataNode(Data)
      , Source({Size, Source})
      , InterpretAsFloat(Float) {}
    Ref StackDataNode {}; // Reference to the data in the Stack.
                          // This is the source data node in the stack format, possibly converted to 64/80 bits.
    struct StackMemberData final {
      OpSize Size;
      Ref Node;
    };
    // Tuple is only valid if we have information about the Source of the Stack Data Node.
    // In it's valid then OpSize is the original source size and Ref is the original source node.
    std::optional<StackMemberData> Source {};
    bool InterpretAsFloat {false}; // True if this is a floating point value, false if integer
  };

  // StackData, TopCache need to be always properly set to ensure
  // they reflect the current state of the FPU. This sync only makes sense while
  // taking the fast path. Once in the slow path, these don't make sense anymore
  // and we are syncing everything.

  // Index on vector is offset to top value at start of block
  // If slow path is true, then StackData is always empty.
  FixedSizeStack<StackMemberInfo> StackData;

  void InvalidateCaches();
  void InvalidateTopOffsetCache();

  // Path Migration helper management
  std::optional<StackMemberInfo> MigrateToSlowPath_IfInvalid(uint8_t Offset = 0);
  Ref LoadStackValue(uint8_t Offset = 0);
  void StoreStackValue(Ref Value, uint8_t Offset = 0, bool SetValid = false);
  void StackPop();

  // Cache for Constants
  // ConstantPoll[i] has IREmit->_Constant(i);
  std::array<Ref, 8> ConstantPool {};
  Ref GetConstant(ssize_t Offset);

  // Cached value for Top
  // If slowpath is false, then TopCache is nullptr.
  bool FlushTopPending = false;
  std::array<Ref, 8> TopOffsetCache {};

  void FlushTop();

  // Are we on the slow path?
  // Once we enter the slow path, we never come out.
  // This just simplifies the code atm. If there's a need to return to the fast path in the future
  // we can implement that but I would expect that there would be very few cases where that's necessary.
  // On the slow path TopCache is always the last obtained version of top.
  // TopOffset is ignored
  bool SlowPath = false;
  // Keeping IREmitter not to pass arguments around
  IREmitter* IREmit = nullptr;
  IRListView* IR = nullptr;
};

inline void X87StackOptimization::InvalidateCaches() {
  InvalidateTopOffsetCache();
  ConstantPool.fill(nullptr);
}

inline void X87StackOptimization::InvalidateTopOffsetCache() {
  FlushTop();
  TopOffsetCache.fill(nullptr);
}

inline void X87StackOptimization::Reset() {
  SlowPath = false;
  StackData.clear();
  InvalidateCaches();
}

inline Ref X87StackOptimization::GetConstant(ssize_t Offset) {
  if (Offset < 0 || Offset >= X87StackOptimization::ConstantPool.size()) {
    // not dealt by pool
    return IREmit->_Constant(Offset);
  }
  if (ConstantPool[Offset] == nullptr) {

    ConstantPool[Offset] = IREmit->_Constant(Offset);
  }
  return ConstantPool[Offset];
}

inline void X87StackOptimization::MigrateToSlowPathIf(bool ShouldMigrate) {
  if (ShouldMigrate && !SlowPath) {
    SynchronizeStackValues();
    StackData.clear();
    SlowPath = true;
  }
}

inline Ref X87StackOptimization::GetTopWithCache_Slow() {
  if (!TopOffsetCache[0]) {
    TopOffsetCache[0] =
      IREmit->_LoadContext(OpSize::i8Bit, GPRClass, offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC);
  }
  return TopOffsetCache[0];
}

inline Ref X87StackOptimization::GetOffsetTopWithCache_Slow(uint8_t Offset, bool Reverse) {
  if (Reverse) {
    Offset = 8 - Offset;
  }

  Offset &= 7;

  if (TopOffsetCache[Offset]) {
    return TopOffsetCache[Offset];
  }

  auto* OffsetTop = GetTopWithCache_Slow();
  if (Offset != 0) {
    OffsetTop = IREmit->_And(OpSize::i32Bit, IREmit->Add(OpSize::i32Bit, OffsetTop, Offset), GetConstant(7));
    // GetTopWithCache_Slow already sets the cache so we don't need to set it here for offset == 0
    TopOffsetCache[Offset] = OffsetTop;
  }

  return OffsetTop;
}


inline void X87StackOptimization::SetTopWithCache_Slow(Ref Value) {
  InvalidateTopOffsetCache();
  TopOffsetCache[0] = Value;
  FlushTopPending = true;
}

inline void X87StackOptimization::SetX87ValidTag(Ref Value, bool Valid) {
  Ref AbridgedFTW = IREmit->_LoadContext(OpSize::i8Bit, GPRClass, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
  Ref RegMask = IREmit->_Lshl(OpSize::i32Bit, GetConstant(1), Value);
  Ref NewAbridgedFTW = Valid ? IREmit->_Or(OpSize::i32Bit, AbridgedFTW, RegMask) : IREmit->_Andn(OpSize::i32Bit, AbridgedFTW, RegMask);
  IREmit->_StoreContext(OpSize::i8Bit, GPRClass, NewAbridgedFTW, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
}

inline Ref X87StackOptimization::GetX87ValidTag_Slow(uint8_t Offset) {
  Ref AbridgedFTW = IREmit->_LoadContext(OpSize::i8Bit, GPRClass, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
  return IREmit->_And(OpSize::i32Bit, IREmit->_Lshr(OpSize::i32Bit, AbridgedFTW, GetOffsetTopWithCache_Slow(Offset)), GetConstant(1));
}

inline Ref X87StackOptimization::LoadStackValueAtOffset_Slow(uint8_t Offset) {
  return IREmit->_LoadContextIndexed(GetOffsetTopWithCache_Slow(Offset), ReducedPrecisionMode ? OpSize::i64Bit : OpSize::i128Bit,
                                     MMBaseOffset(), 16, FPRClass);
}

inline void X87StackOptimization::StoreStackValueAtOffset_Slow(Ref Value, uint8_t Offset, bool SetValid) {
  OrderedNode* TopOffset = GetOffsetTopWithCache_Slow(Offset);
  // store
  IREmit->_StoreContextIndexed(Value, TopOffset, ReducedPrecisionMode ? OpSize::i64Bit : OpSize::i128Bit, MMBaseOffset(), 16, FPRClass);
  // mark it valid
  // In some cases we might already know it has been previously set as valid so we don't need to do it again
  if (SetValid) {
    SetX87ValidTag(TopOffset, true);
  }
}

inline Ref X87StackOptimization::RotateRight8(uint32_t V, Ref Amount) {
  return IREmit->_Lshr(OpSize::i32Bit, GetConstant(V | (V << 8)), Amount);
}

inline std::optional<X87StackOptimization::StackMemberInfo> X87StackOptimization::MigrateToSlowPath_IfInvalid(uint8_t Offset) {
  const auto& [Valid, StackMember] = StackData.top(Offset);
  MigrateToSlowPathIf(Valid != StackSlot::VALID);
  if (Valid == StackSlot::VALID) {
    return StackMember;
  }
  return {};
}

inline Ref X87StackOptimization::LoadStackValue(uint8_t Offset) {
  const auto& StackValue = MigrateToSlowPath_IfInvalid(Offset);
  return SlowPath ? LoadStackValueAtOffset_Slow(Offset) : StackValue->StackDataNode;
}

inline void X87StackOptimization::StoreStackValue(Ref Value, uint8_t Offset, bool SetValid) {
  if (SlowPath) {
    StoreStackValueAtOffset_Slow(Value, Offset, SetValid);
  } else {
    StackData.setTop(StackMemberInfo {Value}, Offset);
  }
}

inline void X87StackOptimization::StackPop() {
  if (SlowPath) {
    UpdateTopForPop_Slow();
  } else {
    StackData.pop();
  }
}


void X87StackOptimization::HandleUnop(IROps Op64, bool VFOp64, IROps Op80) {
  Ref St0 = LoadStackValue();
  Ref Value {};

  if (ReducedPrecisionMode) {
    if (VFOp64) {
      DeriveOp(Value, Op64, IREmit->_VFSqrt(OpSize::i64Bit, OpSize::i64Bit, St0));
    } else {
      DeriveOp(Value, Op64, IREmit->_F64SIN(St0));
    }
  } else {
    DeriveOp(Value, Op80, IREmit->_F80SQRT(St0));
  }

  StoreStackValue(Value);
}


void X87StackOptimization::HandleBinopValue(IROps Op64, bool VFOp64, IROps Op80, uint8_t DestStackOffset, bool MarkDestValid,
                                            uint8_t StackOffset, Ref ValueNode, bool Reverse) {
  LOGMAN_THROW_A_FMT(!Reverse || VFOp64, "There are no reverse operations using non VFOp64 ops");
  auto StackNode = LoadStackValue(StackOffset);

  Ref Node = {};
  if (ReducedPrecisionMode) {
    if (Reverse) {
      DeriveOp(Node, Op64, IREmit->_VFAdd(OpSize::i64Bit, OpSize::i64Bit, ValueNode, StackNode));
    } else {
      if (VFOp64) {
        DeriveOp(Node, Op64, IREmit->_VFAdd(OpSize::i64Bit, OpSize::i64Bit, StackNode, ValueNode));
      } else {
        DeriveOp(Node, Op64, IREmit->_F64FPREM(StackNode, ValueNode));
      }
    }
  } else {
    if (Reverse) {
      DeriveOp(Node, Op80, IREmit->_F80Add(ValueNode, StackNode));
    } else {
      DeriveOp(Node, Op80, IREmit->_F80Add(StackNode, ValueNode));
    }
  }

  StoreStackValue(Node, DestStackOffset, MarkDestValid && StackOffset != DestStackOffset);
}

void X87StackOptimization::HandleBinopStack(IROps Op64, bool VFOp64, IROps Op80, uint8_t DestStackOffset, uint8_t StackOffset1,
                                            uint8_t StackOffset2, bool Reverse) {
  auto StackNode = LoadStackValue(StackOffset2);
  HandleBinopValue(Op64, VFOp64, Op80, DestStackOffset, StackOffset2 != DestStackOffset, StackOffset1, StackNode, Reverse);
}

inline void X87StackOptimization::UpdateTopForPop_Slow() {
  // Pop the top of the x87 stack
  GetOffsetTopWithCache_Slow(1);
  std::rotate(TopOffsetCache.begin(), std::next(TopOffsetCache.begin()), TopOffsetCache.end());
  std::rotate(TopOffsetAddressCache.begin(), std::next(TopOffsetAddressCache.begin()), TopOffsetAddressCache.end());
  FlushTopPending = true;
}

inline void X87StackOptimization::UpdateTopForPush_Slow() {
  // Pop the top of the x87 stack
  GetOffsetTopWithCache_Slow(1, true);
  std::rotate(TopOffsetCache.begin(), std::prev(TopOffsetCache.end()), TopOffsetCache.end());
  std::rotate(TopOffsetAddressCache.begin(), std::prev(TopOffsetAddressCache.end()), TopOffsetAddressCache.end());
  FlushTopPending = true;
}

void X87StackOptimization::FlushTop() {
  if (FlushTopPending) {
    IREmit->_StoreContext(OpSize::i8Bit, GPRClass, TopOffsetCache[0], offsetof(FEXCore::Core::CPUState, flags) + FEXCore::X86State::X87FLAG_TOP_LOC);
    FlushTopPending = false;
  }
}

// We synchronize stack values in a few occasions but one of the most important of those,
// is when we move from fast to a slow path and need to make sure that the context is properly
// written.
Ref X87StackOptimization::SynchronizeStackValues() {
  if (SlowPath) {
    FlushTop();
    return GetTopWithCache_Slow();
  }

  // Store new top which is now the original top minus recorded top offset
  // Careful with underflow wraparound.
  const auto TopOffset = StackData.TopOffset;

  if (TopOffset != 0) {
    Ref NewTop = GetOffsetTopWithCache_Slow(TopOffset, true);
    SetTopWithCache_Slow(NewTop);
  }
  StackData.TopOffset = 0;

  // Before leaving we need to write the current values in the stack to
  // context so that the values are correct. Copy SourceDataNode in the
  // stack to the respective mmX register.
  Ref TopValue = GetTopWithCache_Slow();
  for (size_t i = 0; i < StackData.size; ++i) {
    const auto& [Valid, StackMember] = StackData.top(i);

    if (Valid == StackSlot::UNUSED) {
      continue;
    }
    if (Valid == StackSlot::VALID) {
      StoreStackValueAtOffset_Slow(StackMember.StackDataNode, i, false);
    }
  }
  { // Set valid tags
    uint8_t Mask = StackData.getValidMask();
    if (Mask == 0xff) {
      IREmit->_StoreContext(OpSize::i8Bit, GPRClass, GetConstant(Mask), offsetof(FEXCore::Core::CPUState, AbridgedFTW));
    } else if (Mask != 0) {
      if (std::popcount(Mask) == 1) {
        uint8_t BitIdx = __builtin_ctz(Mask);
        SetX87ValidTag(GetOffsetTopWithCache_Slow(BitIdx), true);
      } else {
        // perform a rotate right on mask by top
        auto* TopValue = GetTopWithCache_Slow();
        Ref RotAmount = IREmit->_Sub(OpSize::i32Bit, GetConstant(8), TopValue);
        Ref AbridgedFTW = IREmit->_LoadContext(OpSize::i8Bit, GPRClass, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
        Ref NewAbridgedFTW = IREmit->_Or(OpSize::i32Bit, AbridgedFTW, RotateRight8(Mask, RotAmount));
        IREmit->_StoreContext(OpSize::i8Bit, GPRClass, NewAbridgedFTW, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
      }
    }
  }
  { // Set invalid tags
    uint8_t Mask = StackData.getInvalidMask();
    if (Mask == 0xff) {
      IREmit->_StoreContext(OpSize::i8Bit, GPRClass, GetConstant(0), offsetof(FEXCore::Core::CPUState, AbridgedFTW));
    } else if (Mask != 0) {
      if (std::popcount(Mask)) {
        uint8_t BitIdx = __builtin_ctz(Mask);
        SetX87ValidTag(GetOffsetTopWithCache_Slow(BitIdx), false);
      } else {
        // Same rotate right as above but this time on the invalid mask
        auto* TopValue = GetTopWithCache_Slow();
        Ref RotAmount = IREmit->_Sub(OpSize::i32Bit, GetConstant(8), TopValue);
        Ref AbridgedFTW = IREmit->_LoadContext(OpSize::i8Bit, GPRClass, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
        Ref NewAbridgedFTW = IREmit->_Andn(OpSize::i32Bit, AbridgedFTW, RotateRight8(Mask, RotAmount));
        IREmit->_StoreContext(OpSize::i8Bit, GPRClass, NewAbridgedFTW, offsetof(FEXCore::Core::CPUState, AbridgedFTW));
      }
    }
  }

  FlushTop();
  return TopValue;
}

void X87StackOptimization::Run(IREmitter* Emit) {
  FEXCORE_PROFILE_SCOPED("PassManager::x87StackOpt");

  auto CurrentIR = Emit->ViewIR();
  auto* HeaderOp = CurrentIR.GetHeader();
  LOGMAN_THROW_A_FMT(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  if (!HeaderOp->HasX87) {
    // If there is no x87 in this, just early exit.
    return;
  }

  // Initialize IREmit member
  IREmit = Emit;
  IR = &CurrentIR;

  // Run optimization proper
  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
    auto BlockIROp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();
    // Each time we deal with a new block we need to start over.
    // The optimization should run per-block
    Reset();

    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      if (!LoweredX87(IROp->Op)) {
        continue;
      }
      IREmit->SetWriteCursor(CodeNode);
      switch (IROp->Op) {
      case OP_F80ADDSTACK: {
        const auto* Op = IROp->C<IROp_F80AddStack>();
        HandleBinopStack(OP_VFADD, true, OP_F80ADD, Op->SrcStack1, Op->SrcStack1, Op->SrcStack2);
        break;
      }

      case OP_F80SUBSTACK: {
        const auto* Op = IROp->C<IROp_F80SubStack>();
        HandleBinopStack(OP_VFSUB, true, OP_F80SUB, Op->DstStack, Op->SrcStack1, Op->SrcStack2);
        break;
      }

      case OP_F80MULSTACK: {
        const auto* Op = IROp->C<IROp_F80MulStack>();
        HandleBinopStack(OP_VFMUL, true, OP_F80MUL, Op->SrcStack1, Op->SrcStack1, Op->SrcStack2);
        break;
      }

      case OP_F80DIVSTACK: {
        const auto* Op = IROp->C<IROp_F80DivStack>();
        HandleBinopStack(OP_VFDIV, true, OP_F80DIV, Op->DstStack, Op->SrcStack1, Op->SrcStack2);
        break;
      }

      case OP_F80FPREMSTACK: {
        HandleBinopStack(OP_F64FPREM, false, OP_F80FPREM, 0, 0, 1);
        break;
      }

      case OP_F80FPREM1STACK: {
        HandleBinopStack(OP_F64FPREM1, false, OP_F80FPREM1, 0, 0, 1);
        break;
      }

      case OP_F80SCALESTACK: {
        HandleBinopStack(OP_F64SCALE, false, OP_F80SCALE, 0, 0, 1);
        break;
      }

      case OP_F80FYL2XSTACK: {
        HandleBinopStack(OP_F64FYL2X, false, OP_F80FYL2X, 1, 0, 1);
        StackPop();
        break;
      }

      case OP_F80ATANSTACK: {
        HandleBinopStack(OP_F64ATAN, false, OP_F80ATAN, 1, 1, 0);
        StackPop();
        break;
      }

      case OP_F80ADDVALUE: {
        const auto* Op = IROp->C<IROp_F80AddValue>();
        HandleBinopValue(OP_VFADD, true, OP_F80ADD, 0, true, Op->SrcStack, CurrentIR.GetNode(Op->X80Src));
        break;
      }

      case OP_F80SUBRVALUE:
      case OP_F80SUBVALUE: {
        const auto* Op = IROp->C<IROp_F80SubValue>();
        HandleBinopValue(OP_VFSUB, true, OP_F80SUB, 0, true, Op->SrcStack, CurrentIR.GetNode(Op->X80Src), IROp->Op == OP_F80SUBRVALUE);
        break;
      }

      case OP_F80DIVRVALUE:
      case OP_F80DIVVALUE: {
        const auto* Op = IROp->C<IROp_F80DivValue>();
        HandleBinopValue(OP_VFDIV, true, OP_F80DIV, 0, true, Op->SrcStack, CurrentIR.GetNode(Op->X80Src), IROp->Op == OP_F80DIVRVALUE);
        break;
      }

      case OP_F80MULVALUE: {
        const auto* Op = IROp->C<IROp_F80MulValue>();
        HandleBinopValue(OP_VFMUL, true, OP_F80MUL, 0, true, Op->SrcStack, CurrentIR.GetNode(Op->X80Src));
        break;
      }

      case OP_F80SQRTSTACK: {
        HandleUnop(OP_VFSQRT, true, OP_F80SQRT);
        break;
      }

      case OP_F80SINSTACK: {
        HandleUnop(OP_F64SIN, false, OP_F80SIN);
        break;
      }

      case OP_F80COSSTACK: {
        HandleUnop(OP_F64COS, false, OP_F80COS);
        break;
      }

      case OP_F80F2XM1STACK: {
        HandleUnop(OP_F64F2XM1, false, OP_F80F2XM1);
        break;
      }


      case OP_F80PTANSTACK: {
        HandleUnop(OP_F64TAN, false, OP_F80TAN);
        Ref OneConst {};
        if (ReducedPrecisionMode) {
          OneConst = IREmit->_VCastFromGPR(OpSize::i64Bit, OpSize::i64Bit, GetConstant(0x3FF0000000000000));
        } else {
          OneConst = IREmit->_LoadNamedVectorConstant(OpSize::i128Bit, NamedVectorConstant::NAMED_VECTOR_X87_ONE);
        }

        if (SlowPath) {
          UpdateTopForPush_Slow();
          StoreStackValueAtOffset_Slow(OneConst);
        } else {
          StackData.push(StackMemberInfo {OneConst});
        }
        break;
      }

      case OP_F80SINCOSSTACK: {
        Ref St0 = LoadStackValue();

        Ref SinValue {};
        Ref CosValue {};

#ifdef VIXL_SIMULATOR
        if (DisableVixlIndirectCalls() == 0) {
          if (ReducedPrecisionMode) {
            SinValue = IREmit->_F64SIN(St0);
            CosValue = IREmit->_F64COS(St0);
          } else {
            SinValue = IREmit->_F80SIN(St0);
            CosValue = IREmit->_F80COS(St0);
          }
        } else
#endif
        {
          SinValue = IREmit->_AllocateFPR(OpSize::i128Bit, OpSize::i128Bit);
          CosValue = IREmit->_AllocateFPR(OpSize::i128Bit, OpSize::i128Bit);
          if (ReducedPrecisionMode) {
            IREmit->_F64SINCOS(St0, SinValue, CosValue);
          } else {
            IREmit->_F80SINCOS(St0, SinValue, CosValue);
          }
        }

        // Push values
        if (SlowPath) {
          StoreStackValueAtOffset_Slow(SinValue, 0, false);
          UpdateTopForPush_Slow();
          StoreStackValueAtOffset_Slow(CosValue, 0, true);
        } else {
          StackData.setTop(StackMemberInfo {SinValue});
          StackData.push(StackMemberInfo {CosValue});
        }
        break;
      }

      case OP_INITSTACK: {
        StackData.clear();
        InvalidateTopOffsetCache();
        break;
      }

      case OP_INVALIDATESTACK: {
        const auto* Op = IROp->C<IROp_ReadStackValue>();
        auto Offset = Op->StackLocation;

        if (Offset != 0xff) { // invalidate single offset
          if (SlowPath) {
            auto* TopValue = GetOffsetTopWithCache_Slow(Offset);
            SetX87ValidTag(TopValue, false);
          } else {
            StackData.setTagInvalid(Offset);
          }
        } else { // invalidate all
          if (SlowPath) {
            IREmit->_StoreContext(OpSize::i8Bit, GPRClass, GetConstant(0), offsetof(FEXCore::Core::CPUState, AbridgedFTW));
          } else {
            for (size_t i = 0; i < StackData.size; i++) {
              StackData.setTagInvalid(i);
            }
          }
        }
        break;
      }

      case OP_PUSHSTACK: {
        const auto* Op = IROp->C<IROp_PushStack>();
        auto* SourceNode = CurrentIR.GetNode(Op->X80Src);

        if (SlowPath) {
          UpdateTopForPush_Slow();
          StoreStackValueAtOffset_Slow(SourceNode);
        } else {
          auto* SourceNode = CurrentIR.GetNode(Op->X80Src);
          auto* OriginalNode = CurrentIR.GetNode(Op->OriginalValue);
          StackData.push(StackMemberInfo {SourceNode, OriginalNode, Op->LoadSize, Op->Float});
        }
        break;
      }

      case OP_COPYPUSHSTACK: {
        const auto* Op = IROp->C<IROp_CopyPushStack>();
        auto Offset = Op->StackLocation;
        auto Value = MigrateToSlowPath_IfInvalid(Offset);

        if (SlowPath) {
          Ref St0 = LoadStackValueAtOffset_Slow(Offset);
          UpdateTopForPush_Slow();
          StoreStackValueAtOffset_Slow(St0);
        } else {
          StackData.push(*Value);
        }
        break;
      }

      case OP_READSTACKVALUE: {
        const auto* Op = IROp->C<IROp_ReadStackValue>();
        auto Offset = Op->StackLocation;
        Ref NewValue = LoadStackValue(Offset);

        IREmit->ReplaceUsesWithAfter(CodeNode, NewValue, CodeNode);
        break;
      }

      case OP_STACKVALIDTAG: {
        // Returns 0 if value is valid and 1 otherwise.
        const auto* Op = IROp->C<IROp_StackValidTag>();
        auto Offset = Op->StackLocation;
        auto Value = MigrateToSlowPath_IfInvalid(Offset);

        Ref Tag {};
        if (SlowPath) {
          Tag = GetX87ValidTag_Slow(Offset);
        } else {
          Tag = Value ? GetConstant(1) : GetConstant(0);
        }

        IREmit->ReplaceUsesWithAfter(CodeNode, Tag, CodeNode);
        break;
      }

      case OP_STORESTACKMEM: {
        const auto* Op = IROp->C<IROp_StoreStackMem>();
        const auto& Value = MigrateToSlowPath_IfInvalid();
        Ref StackNode = SlowPath ? LoadStackValueAtOffset_Slow() : Value->StackDataNode;
        Ref AddrNode = CurrentIR.GetNode(Op->Addr);
        Ref Offset = CurrentIR.GetNode(Op->Offset);
        OpSize Align = Op->Align;
        MemOffsetType OffsetType = Op->OffsetType;
        uint8_t OffsetScale = Op->OffsetScale;

        // On the fast path we can optimize memory copies.
        // If we are doing:
        // fld dword [rax]
        // fst dword [rbx]
        // We can optimize this to:
        // ldr w2, [x0]
        // str w2, [x1]
        // or similar. As long as the source size and dest size are one and the same.
        // This will avoid any conversions between source and stack element size and conversion back.
        if (!SlowPath && Value->Source && Value->Source->Size == Op->StoreSize && Value->InterpretAsFloat) {
          IREmit->_StoreMem(Value->InterpretAsFloat ? FPRClass : GPRClass, Op->StoreSize, Value->Source->Node, AddrNode, Offset, Align,
                            OffsetType, OffsetScale);
          break;
        }

        if (ReducedPrecisionMode) {
          StoreStackMem_Reduced_Helper(Op, StackNode);
          break;
        }

        StoreStackMem_Helper(Op, StackNode);
        break;
      }

      case OP_STORESTACKTOSTACK: { // stores top of stack in another place in stack.
        const auto* Op = IROp->C<IROp_StoreStackToStack>();
        auto Offset = Op->StackLocation;

        if (Offset != 0) {
          auto Value = MigrateToSlowPath_IfInvalid();

          // Need to store st0 to stack location - basically a copy.
          if (SlowPath) {
            StoreStackValueAtOffset_Slow(LoadStackValueAtOffset_Slow(), Offset);
          } else {
            StackData.setTop(*Value, Offset);
          }
        }
        break;
      }
      case OP_POPSTACKDESTROY: {
        if (SlowPath) {
          SetX87ValidTag(GetTopWithCache_Slow(), false);
        }
        StackPop();
        break;
      }

      case OP_F80STACKXCHANGE: {
        const auto* Op = IROp->C<IROp_F80StackXchange>();
        auto Offset = Op->SrcStack;
        Ref ValueTop = LoadStackValue();
        Ref ValueOffset = LoadStackValue(Offset);

        StoreStackValue(ValueOffset);
        StoreStackValue(ValueTop, Offset);
        break;
      }

      case OP_F80STACKCHANGESIGN: {
        Ref Value = LoadStackValue();

        // We need a couple of intermediate instructions to change the sign
        // of a value
        Ref ResultNode {};
        if (ReducedPrecisionMode) {
          ResultNode = IREmit->_VFNeg(OpSize::i64Bit, OpSize::i64Bit, Value);
        } else {
          Ref HelperNode = IREmit->_LoadNamedVectorConstant(OpSize::i128Bit, IR::NamedVectorConstant::NAMED_VECTOR_F80_SIGN_MASK);
          ResultNode = IREmit->_VXor(OpSize::i128Bit, OpSize::i8Bit, Value, HelperNode);
        }
        StoreStackValue(ResultNode);
        break;
      }

      case OP_F80STACKABS: {
        Ref Value = LoadStackValue();

        Ref ResultNode {};
        if (ReducedPrecisionMode) {
          ResultNode = IREmit->_VFAbs(OpSize::i64Bit, OpSize::i64Bit, Value);
        } else {
          // Intermediate insts
          Ref HelperNode = IREmit->_LoadNamedVectorConstant(OpSize::i128Bit, IR::NamedVectorConstant::NAMED_VECTOR_F80_SIGN_MASK);
          ResultNode = IREmit->_VAndn(OpSize::i128Bit, OpSize::i8Bit, Value, HelperNode);
        }
        StoreStackValue(ResultNode);
        break;
      }

      case OP_F80CMPSTACK: {
        const auto* Op = IROp->C<IROp_F80CmpStack>();
        auto Offset = Op->SrcStack;
        Ref StackValue1 = LoadStackValue();
        Ref StackValue2 = LoadStackValue(Offset);

        Ref CmpNode {};
        if (ReducedPrecisionMode) {
          CmpNode = IREmit->_FCmp(OpSize::i64Bit, StackValue1, StackValue2);
        } else {
          CmpNode = IREmit->_F80Cmp(StackValue1, StackValue2);
        }

        IREmit->ReplaceUsesWithAfter(CodeNode, CmpNode, CodeNode);
        break;
      }
      case OP_F80STACKTEST: {
        const auto* Op = IROp->C<IROp_F80StackTest>();
        auto Offset = Op->SrcStack;
        auto StackNode = LoadStackValue(Offset);
        Ref ZeroConst = IREmit->_VCastFromGPR(ReducedPrecisionMode ? OpSize::i64Bit : OpSize::i128Bit, OpSize::i64Bit, GetConstant(0));

        Ref CmpNode {};
        if (ReducedPrecisionMode) {
          CmpNode = IREmit->_FCmp(OpSize::i64Bit, StackNode, ZeroConst);
        } else {
          CmpNode = IREmit->_F80Cmp(StackNode, ZeroConst);
        }
        IREmit->ReplaceUsesWithAfter(CodeNode, CmpNode, CodeNode);
        break;
      }


      case OP_F80CMPVALUE: {
        const auto* Op = IROp->C<IROp_F80CmpValue>();
        const auto& Value = CurrentIR.GetNode(Op->X80Src);
        auto StackNode = LoadStackValue();

        Ref CmpNode {};
        if (ReducedPrecisionMode) {
          CmpNode = IREmit->_FCmp(OpSize::i64Bit, StackNode, Value);
        } else {
          CmpNode = IREmit->_F80Cmp(StackNode, Value);
        }
        IREmit->ReplaceUsesWithAfter(CodeNode, CmpNode, CodeNode);
        break;
      }

      case OP_SYNCSTACKTOSLOW: {
        // This synchronizes stack values but doesn't necessarily moves us off the FastPath!
        Ref NewTop = SynchronizeStackValues();
        IREmit->ReplaceUsesWithAfter(CodeNode, NewTop, CodeNode);
        break;
      }

      case OP_STACKFORCESLOW: {
        MigrateToSlowPathIf(true);
        InvalidateTopOffsetCache();
        break;
      }

      case OP_INCSTACKTOP: {
        if (SlowPath) {
          UpdateTopForPop_Slow();
        } else {
          StackData.rotate(false);
        }
        break;
      }

      case OP_DECSTACKTOP: {
        if (SlowPath) {
          UpdateTopForPush_Slow();
        } else {
          StackData.rotate(true);
        }
        break;
      }

      case OP_F80ROUNDSTACK: {
        Ref St0 = LoadStackValue();

        Ref Value {};
        if (ReducedPrecisionMode) {
          Value = IREmit->_Vector_FToI(OpSize::i64Bit, OpSize::i64Bit, St0, Round_Host);
        } else {
          Value = IREmit->_F80Round(St0);
        }
        StoreStackValue(Value);
        break;
      }

      case OP_F80VBSLSTACK: {
        const auto* Op = IROp->C<IROp_F80VBSLStack>();

        auto StackOffset1 = Op->SrcStack1;
        auto StackOffset2 = Op->SrcStack2;
        Ref Value1 = LoadStackValue(StackOffset1);
        Ref Value2 = LoadStackValue(StackOffset2);

        Ref StackNode = IREmit->_VBSL(OpSize::i128Bit, CurrentIR.GetNode(Op->VectorMask), Value1, Value2);
        StoreStackValue(StackNode, 0, StackOffset1 && StackOffset2);
        break;
      }

      default: LOGMAN_THROW_A_FMT(false, "IROp was expected to be lowered");
      }
      IREmit->Remove(CodeNode);
    }

    auto Last = CurrentIR.at(BlockIROp->Last);
    --Last;
    auto [LastCodeNode, LastIROp] = Last();
    LOGMAN_THROW_A_FMT(IsBlockExit(LastIROp->Op), "must be exit");
    IREmit->SetWriteCursorBefore(LastCodeNode);
    SynchronizeStackValues();
  }

  return;
}

fextl::unique_ptr<Pass> CreateX87StackOptimizationPass(const FEXCore::HostFeatures& Features, OpSize GPROpSize) {
  return fextl::make_unique<X87StackOptimization>(Features, GPROpSize);
}
} // namespace FEXCore::IR
