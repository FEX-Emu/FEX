// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/ThreadPoolAllocator.h>
#include <FEXCore/IR/IR.h>

#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/sstream.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <type_traits>

namespace FEXCore::IR {

class OrderedNode;
class RegisterAllocationPass;

/**
 * @brief The IROp_Header is an dynamically sized array
 * At the end it contains a uint8_t for the number of arguments that Op has
 * Then there is an unsized array of NodeWrapper arguments for the number of arguments this op has
 * The op structures that are including the header must ensure that they pad themselves correctly to the number of arguments used
 */
struct IROp_Header;

/**
 * @brief Represents the ID of a given IR node.
 *
 * Intended to provide strong typing from other integer values
 * to prevent passing incorrect values to certain API functions.
 */
struct NodeID final {
  using value_type = uint32_t;

  constexpr NodeID() noexcept = default;
  constexpr explicit NodeID(value_type Value_) noexcept
    : Value {Value_} {}

  constexpr NodeID(const NodeID&) noexcept = default;
  constexpr NodeID& operator=(const NodeID&) noexcept = default;

  constexpr NodeID(NodeID&&) noexcept = default;
  constexpr NodeID& operator=(NodeID&&) noexcept = default;

  [[nodiscard]]
  constexpr bool IsValid() const noexcept {
    return Value != 0;
  }
  [[nodiscard]]
  constexpr bool IsInvalid() const noexcept {
    return !IsValid();
  }
  constexpr void Invalidate() noexcept {
    Value = 0;
  }

  [[nodiscard]] friend constexpr bool operator==(NodeID, NodeID) noexcept = default;

  [[nodiscard]]
  friend constexpr bool operator<(NodeID lhs, NodeID rhs) noexcept {
    return lhs.Value < rhs.Value;
  }
  [[nodiscard]]
  friend constexpr bool operator>(NodeID lhs, NodeID rhs) noexcept {
    return operator<(rhs, lhs);
  }
  [[nodiscard]]
  friend constexpr bool operator<=(NodeID lhs, NodeID rhs) noexcept {
    return !operator>(lhs, rhs);
  }
  [[nodiscard]]
  friend constexpr bool operator>=(NodeID lhs, NodeID rhs) noexcept {
    return !operator<(lhs, rhs);
  }

  friend std::ostream& operator<<(std::ostream& out, NodeID ID) {
    out << ID.Value;
    return out;
  }
  friend std::istream& operator>>(std::istream& in, NodeID& ID) {
    in >> ID.Value;
    return in;
  }

  value_type Value {};
};

/**
 * @brief This is a very simple wrapper for our node pointers
 * You probably don't want to use this directly
 * Use OpNodeWrapper and OrderedNodeWrapper types below instead
 *
 * This is necessary to allow two things
 *  - Reduce memory usage by having the pointer be an 32bit offset rather than the whole 64bit pointer
 *  - Actually use an offset from a base so we aren't storing pointers for everything
 *    - Makes IR list copying be as cheap as a memcpy
 * Downsides
 *  - The IR nodes have to be allocated out of a linear array of memory
 *  - We currently only allow a 32bit offset, so *only* 4 million nodes per list
 *  - We have to have the base offset live somewhere else
 *  - Has to be POD and trivially copyable
 *  - Makes every real node access turn in to a [Base + Offset] access
 *  - Can be confusing if you're mixing OpNodeWrapper and OrderedNodeWrapper usage
 */
template<typename Type>
struct FEX_PACKED NodeWrapperBase final {
  // 32bit or 64bit offset doesn't matter for addressing.
  // We use uint32_t to be more memory efficient (Cuts our node list size in half)
  using NodeOffsetType = uint32_t;
  NodeOffsetType NodeOffset;

  explicit NodeWrapperBase() = default;

  [[nodiscard]]
  static NodeWrapperBase WrapOffset(NodeOffsetType Offset) {
    NodeWrapperBase Wrapped;
    Wrapped.NodeOffset = Offset;
    return Wrapped;
  }

  [[nodiscard]]
  static NodeWrapperBase WrapPtr(uintptr_t Base, uintptr_t Value) {
    NodeWrapperBase Wrapped;
    Wrapped.SetOffset(Base, Value);
    return Wrapped;
  }

  [[nodiscard]]
  static void* UnwrapNode(uintptr_t Base, NodeWrapperBase Node) {
    return Node.GetNode(Base);
  }

  [[nodiscard]]
  NodeID ID() const;

  [[nodiscard]]
  bool IsInvalid() const {
    return NodeOffset == 0;
  }

  [[nodiscard]]
  bool IsImmediate() const {
    return NodeOffset & (1u << 31);
  }

  [[nodiscard]]
  bool HasKill() const {
    return NodeOffset & (1u << 30);
  }

  void ClearKill() {
    NodeOffset &= ~(1u << 30);
  }

  void SetKill() {
    NodeOffset |= (1u << 30);
  }

  [[nodiscard]]
  bool IsPointer() const {
    return !IsImmediate() && !HasKill();
  }

  [[nodiscard]]
  Type* GetNode(uintptr_t Base) {
    LOGMAN_THROW_A_FMT(IsPointer(), "Precondition");
    return reinterpret_cast<Type*>(Base + NodeOffset);
  }
  [[nodiscard]]
  const Type* GetNode(uintptr_t Base) const {
    LOGMAN_THROW_A_FMT(IsPointer(), "Precondition");
    return reinterpret_cast<const Type*>(Base + NodeOffset);
  }

  void SetOffset(uintptr_t Base, uintptr_t Value) {
    NodeOffset = Value - Base;
    LOGMAN_THROW_A_FMT(IsPointer(), "Offsets are within 2GiB range");
  }

  void SetInvalid() {
    NodeOffset = 0;
    LOGMAN_THROW_A_FMT(IsInvalid(), "Zero state");
  }

  void SetImmediate(uint32_t Immediate) {
    LOGMAN_THROW_A_FMT(Immediate < (1u << 31), "Bounded");
    NodeOffset = Immediate | (1u << 31);
    LOGMAN_THROW_A_FMT(IsImmediate(), "Encoded above");
  }

  [[nodiscard]]
  uint32_t GetImmediate() const {
    LOGMAN_THROW_A_FMT(IsImmediate(), "Precondition: must be an immediate");
    return NodeOffset & ~(1u << 31);
  }

  [[nodiscard]]
  friend constexpr bool operator==(const NodeWrapperBase<Type>&, const NodeWrapperBase<Type>&) = default;

  [[nodiscard]]
  static NodeWrapperBase<Type> FromImmediate(uint32_t Immediate) {
    NodeWrapperBase<Type> A;
    A.SetImmediate(Immediate);
    return A;
  }
};

static_assert(std::is_trivially_copyable_v<NodeWrapperBase<OrderedNode>>);

static_assert(sizeof(NodeWrapperBase<OrderedNode>) == sizeof(uint32_t));

using OpNodeWrapper = NodeWrapperBase<IROp_Header>;
using OrderedNodeWrapper = NodeWrapperBase<OrderedNode>;

struct OrderedNodeHeader {
  OpNodeWrapper Value;
  OrderedNodeWrapper Next;
  OrderedNodeWrapper Previous;
};

static_assert(sizeof(OrderedNodeHeader) == sizeof(uint32_t) * 3);

/**
 * @brief This is a node in our IR representation
 * Is a doubly linked list node that lives in a representation of a linearly allocated node list
 * The links in the nodes can live in a list independent of the data IR data
 *
 * ex.
 *  Region1 : ... <-> <OrderedNode> <-> <OrderedNode> <-> ...
 *                    | *<Value>        |
 *                    v                 v
 *  Region2 : <IROp>..<IROp>..<IROp>..<IROp>
 *
 *  In this example the OrderedNodes are allocated in one linear memory region (Not necessarily contiguous with one another linking)
 *  The second region is contiguous but they don't have any relationship with one another directly
 */
class OrderedNode final {
public:
  // These three values are laid out very specifically to make it fast to access the NodeWrappers specifically
  OrderedNodeHeader Header;
  uint32_t NumUses;

  // After RA, the register allocated for the node. This is the register for the
  // node at the time it is written, even if it is shuffled into other registers
  // later. In other words, it is the register destination of the instruction
  // represented by this OrderedNode.
  //
  // This is the raw value of a PhysicalRegister data structure.
  uint8_t Reg;
  uint8_t Pad[3];

  using value_type = OrderedNodeWrapper;

  OrderedNode() = default;

  /**
   * @brief Appends a node to this current node
   *
   * Before. <Prev> <-> <Current> <-> <Next>
   * After.  <Prev> <-> <Current> <-> <Node> <-> Next
   *
   * @return Pointer to the node being added
   */
  value_type append(uintptr_t Base, value_type Node) {
    // Set Next Node's Previous to incoming node
    SetPrevious(Base, Header.Next, Node);

    // Set Incoming node's links to this node's links
    SetPrevious(Base, Node, Wrapped(Base));
    SetNext(Base, Node, Header.Next);

    // Set this node's next to the incoming node
    SetNext(Base, Wrapped(Base), Node);

    // Return the node we are appending
    return Node;
  }

  OrderedNode* append(uintptr_t Base, OrderedNode* Node) {
    value_type WNode = Node->Wrapped(Base);
    // Set Next Node's Previous to incoming node
    SetPrevious(Base, Header.Next, WNode);

    // Set Incoming node's links to this node's links
    SetPrevious(Base, WNode, Wrapped(Base));
    SetNext(Base, WNode, Header.Next);

    // Set this node's next to the incoming node
    SetNext(Base, Wrapped(Base), WNode);

    // Return the node we are appending
    return Node;
  }

  /**
   * @brief Prepends a node to the current node
   * Before. <Prev> <-> <Current> <-> <Next>
   * After.  <Prev> <-> <Node> <-> <Current> <-> Next
   *
   * @return Pointer to the node being added
   */
  value_type prepend(uintptr_t Base, value_type Node) {
    // Set the previous node's next to the incoming node
    SetNext(Base, Header.Previous, Node);

    // Set the incoming node's links
    SetPrevious(Base, Node, Header.Previous);
    SetNext(Base, Node, Wrapped(Base));

    // Set the current node's link
    SetPrevious(Base, Wrapped(Base), Node);

    // Return the node we are prepending
    return Node;
  }

  OrderedNode* prepend(uintptr_t Base, OrderedNode* Node) {
    value_type WNode = Node->Wrapped(Base);
    // Set the previous node's next to the incoming node
    SetNext(Base, Header.Previous, WNode);

    // Set the incoming node's links
    SetPrevious(Base, WNode, Header.Previous);
    SetNext(Base, WNode, Wrapped(Base));

    // Set the current node's link
    SetPrevious(Base, Wrapped(Base), WNode);

    // Return the node we are prepending
    return Node;
  }

  /**
   * @brief Gets the remaining size of the blocks from this point onward
   *
   * Doesn't find the head of the list
   *
   */
  [[nodiscard]]
  size_t size(uintptr_t Base) const {
    size_t Size = 1;
    // Walk the list forward until we hit a sentinel
    value_type Current = Header.Next;
    while (Current.NodeOffset != 0) {
      ++Size;
      OrderedNode* RealNode = Current.GetNode(Base);
      Current = RealNode->Header.Next;
    }
    return Size;
  }

  void Unlink(uintptr_t Base) {
    // This removes the node from the list. Orphaning it
    // Before: <Previous> <-> <Current> <-> <Next>
    // After: <Previous <-> <Next>
    SetNext(Base, Header.Previous, Header.Next);
    SetPrevious(Base, Header.Next, Header.Previous);
  }

  [[nodiscard]]
  const IROp_Header* Op(uintptr_t Base) const {
    return Header.Value.GetNode(Base);
  }
  [[nodiscard]]
  IROp_Header* Op(uintptr_t Base) {
    return Header.Value.GetNode(Base);
  }

  [[nodiscard]]
  uint32_t GetUses() const {
    return NumUses;
  }

  void AddUse() {
    ++NumUses;
  }
  void RemoveUse() {
    --NumUses;
  }

  [[nodiscard]]
  value_type Wrapped(uintptr_t Base) const {
    value_type Tmp;
    Tmp.SetOffset(Base, reinterpret_cast<uintptr_t>(this));
    return Tmp;
  }

private:
  [[nodiscard]]
  value_type WrappedOffset(uint32_t Offset) const {
    value_type Tmp;
    Tmp.NodeOffset = Offset;
    return Tmp;
  }

  static void SetPrevious(uintptr_t Base, value_type Node, value_type New) {
    OrderedNode* RealNode = Node.GetNode(Base);
    RealNode->Header.Previous = New;
  }

  static void SetNext(uintptr_t Base, value_type Node, value_type New) {
    OrderedNode* RealNode = Node.GetNode(Base);
    RealNode->Header.Next = New;
  }

  void SetUses(uint32_t Uses) {
    NumUses = Uses;
  }
};

static_assert(std::is_trivially_constructible_v<OrderedNode>);
static_assert(std::is_trivially_copyable_v<OrderedNode>);
static_assert(offsetof(OrderedNode, Header) == 0);
static_assert(sizeof(OrderedNode) == (sizeof(OrderedNodeHeader) + 2 * sizeof(uint32_t)));

// This is temporary. We are transitioning away from OrderedNode's in favour of
// flat Ref words. To ease porting, we have this typedef. Eventually OrderedNode
// will be removed and this typedef will be replaced by something like:
//
//  struct Ref {
//     uint Flags : 1;
//     uint ID : 23;
//     uint Reg : 8;
//  };
using Ref = OrderedNode*;

struct FEX_PACKED RegisterClassType final {
  using value_type = uint32_t;

  value_type Val;
  [[nodiscard]] constexpr operator value_type() const {
    return Val;
  }
  [[nodiscard]]
  friend constexpr bool operator==(const RegisterClassType&, const RegisterClassType&) = default;
};

struct FEX_PACKED TypeDefinition final {
  uint16_t Val;

  [[nodiscard]] constexpr operator uint16_t() const {
    return Val;
  }

  [[nodiscard]]
  static constexpr TypeDefinition Create(uint8_t Bytes) {
    TypeDefinition Type {};
    Type.Val = Bytes << 8;
    return Type;
  }

  [[nodiscard]]
  static constexpr TypeDefinition Create(uint8_t Bytes, uint8_t Elements) {
    TypeDefinition Type {};
    Type.Val = (Bytes << 8) | (Elements & 255);
    return Type;
  }

  [[nodiscard]]
  constexpr uint8_t Bytes() const {
    return Val >> 8;
  }

  [[nodiscard]]
  constexpr uint8_t Elements() const {
    return Val & 255;
  }

  [[nodiscard]]
  friend constexpr bool operator==(const TypeDefinition&, const TypeDefinition&) = default;
};

static_assert(std::is_trivially_copyable_v<TypeDefinition>);

class NodeIterator;

/* This iterator can be used to step though nodes.
 * Due to how our IR is laid out, this can be used to either step
 * though the CodeBlocks or though the code within a single block.
 */
class NodeIterator {
public:
  struct value_type final {
    OrderedNode* Node;
    IROp_Header* Header;
  };
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = NodeIterator;
  using const_iterator = const NodeIterator;
  using reverse_iterator = iterator;
  using const_reverse_iterator = const_iterator;
  using iterator_category = std::bidirectional_iterator_tag;

  NodeIterator(uintptr_t Base, uintptr_t IRBase)
    : BaseList {Base}
    , IRList {IRBase} {}
  explicit NodeIterator(uintptr_t Base, uintptr_t IRBase, OrderedNodeWrapper Ptr)
    : BaseList {Base}
    , IRList {IRBase}
    , Node {Ptr} {}

  [[nodiscard]]
  bool operator==(const NodeIterator& rhs) const {
    return Node.NodeOffset == rhs.Node.NodeOffset;
  }

  [[nodiscard]]
  bool operator!=(const NodeIterator& rhs) const {
    return !operator==(rhs);
  }

  NodeIterator operator++() {
    OrderedNodeHeader* RealNode = reinterpret_cast<OrderedNodeHeader*>(Node.GetNode(BaseList));
    Node = RealNode->Next;
    return *this;
  }

  NodeIterator operator--() {
    OrderedNodeHeader* RealNode = reinterpret_cast<OrderedNodeHeader*>(Node.GetNode(BaseList));
    Node = RealNode->Previous;
    return *this;
  }

  [[nodiscard]]
  value_type operator*() {
    OrderedNode* RealNode = Node.GetNode(BaseList);
    return {RealNode, RealNode->Op(IRList)};
  }

  [[nodiscard]]
  value_type operator()() {
    OrderedNode* RealNode = Node.GetNode(BaseList);
    return {RealNode, RealNode->Op(IRList)};
  }

  [[nodiscard]]
  NodeID ID() const {
    return Node.ID();
  }

  [[nodiscard]]
  static NodeIterator Invalid() {
    return NodeIterator(0, 0);
  }

protected:
  uintptr_t BaseList {};
  uintptr_t IRList {};
  OrderedNodeWrapper Node {};
};

// This must directly match bytes to the named opsize.
// Implicit sized IR operations does math to get between sizes.
enum class OpSize : uint8_t {
  iUnsized = 0,
  i8Bit = 1,
  i16Bit = 2,
  i32Bit = 4,
  i64Bit = 8,
  f80Bit = 10,
  i128Bit = 16,
  i256Bit = 32,
  iInvalid = 0xFF,
};

enum class FloatCompareOp : uint8_t {
  EQ = 0,
  LT,
  LE,
  UNO,
  NEQ,
  ORD,
};

enum class ShiftType : uint8_t {
  LSL = 0,
  LSR,
  ASR,
  ROR,
};

enum class BranchHint : uint8_t { None = 0, Call, Return, CheckTF };


// Converts a size stored as an integer in to an OpSize enum.
// This is a nop operation and will be eliminated by the compiler.
static inline OpSize SizeToOpSize(uint8_t Size) {
  switch (Size) {
  case 0: return OpSize::iUnsized;
  case 1: return OpSize::i8Bit;
  case 2: return OpSize::i16Bit;
  case 4: return OpSize::i32Bit;
  case 8: return OpSize::i64Bit;
  case 10: return OpSize::f80Bit;
  case 16: return OpSize::i128Bit;
  case 32: return OpSize::i256Bit;
  case 0xFF: return OpSize::iInvalid;
  default: FEX_UNREACHABLE;
  }
}

// This is a nop operation and will be eliminated by the compiler.
static inline uint8_t OpSizeToSize(IR::OpSize Size) {
  switch (Size) {
  case OpSize::iUnsized: return 0;
  case OpSize::i8Bit: return 1;
  case OpSize::i16Bit: return 2;
  case OpSize::i32Bit: return 4;
  case OpSize::i64Bit: return 8;
  case OpSize::f80Bit: return 10;
  case OpSize::i128Bit: return 16;
  case OpSize::i256Bit: return 32;
  case OpSize::iInvalid: return 0xFF;
  default: FEX_UNREACHABLE;
  }
}

static inline uint16_t OpSizeAsBits(IR::OpSize Size) {
  LOGMAN_THROW_A_FMT(Size != IR::OpSize::iInvalid, "Invalid Size");
  return IR::OpSizeToSize(Size) * 8u;
}

template<typename T>
requires (std::is_integral_v<T>)
static inline OpSize operator<<(IR::OpSize Size, T Shift) {
  LOGMAN_THROW_A_FMT(Size != IR::OpSize::iInvalid, "Invalid Size");
  return IR::SizeToOpSize(IR::OpSizeToSize(Size) << Shift);
}

template<typename T>
requires (std::is_integral_v<T>)
static inline OpSize operator>>(IR::OpSize Size, T Shift) {
  LOGMAN_THROW_A_FMT(Size != IR::OpSize::iInvalid, "Invalid Size");
  return IR::SizeToOpSize(IR::OpSizeToSize(Size) >> Shift);
}

static inline OpSize operator/(IR::OpSize Size, IR::OpSize Divisor) {
  LOGMAN_THROW_A_FMT(Size != IR::OpSize::iInvalid, "Invalid Size");
  return IR::SizeToOpSize(IR::OpSizeToSize(Size) / IR::OpSizeToSize(Divisor));
}

template<typename T>
requires (std::is_integral_v<T>)
static inline OpSize operator/(IR::OpSize Size, T Divisor) {
  LOGMAN_THROW_A_FMT(Size != IR::OpSize::iInvalid, "Invalid Size");
  return IR::SizeToOpSize(IR::OpSizeToSize(Size) / Divisor);
}

static inline uint8_t NumElements(IR::OpSize RegisterSize, IR::OpSize ElementSize) {
  LOGMAN_THROW_A_FMT(RegisterSize != IR::OpSize::iInvalid && ElementSize != IR::OpSize::iInvalid && RegisterSize != IR::OpSize::iUnsized &&
                       ElementSize != IR::OpSize::iUnsized,
                     "Invalid Size");
  return IR::OpSizeToSize(RegisterSize) / IR::OpSizeToSize(ElementSize);
}

#define IROP_ENUM
#define IROP_STRUCTS
#define IROP_SIZES
#define IROP_REG_CLASSES
#include <FEXCore/IR/IRDefines.inc>

/* This iterator can be used to step though every single node in a multi-block in SSA order.
 *
 * Iterates in the order of:
 *
 * end <-- CodeBlockA <--> BlockAInst1 <--> BlockAInst2 <--> CodeBlockB <--> BlockBInst1 <--> BlockBInst2 --> end
 */
class AllNodesIterator : public NodeIterator {
public:
  AllNodesIterator(uintptr_t Base, uintptr_t IRBase)
    : NodeIterator(Base, IRBase) {}
  explicit AllNodesIterator(uintptr_t Base, uintptr_t IRBase, OrderedNodeWrapper Ptr)
    : NodeIterator(Base, IRBase, Ptr) {}
  AllNodesIterator(NodeIterator other)
    : NodeIterator(other) {} // Allow NodeIterator to be upgraded

  AllNodesIterator operator++() {
    OrderedNodeHeader* RealNode = reinterpret_cast<OrderedNodeHeader*>(Node.GetNode(BaseList));
    auto IROp = Node.GetNode(BaseList)->Op(IRList);

    // If this is the last node of a codeblock, we need to continue to the next block
    if (IROp->Op == OP_ENDBLOCK) {
      auto EndBlock = IROp->C<IROp_EndBlock>();

      auto CurrentBlock = EndBlock->BlockHeader.GetNode(BaseList);
      Node = CurrentBlock->Header.Next;
    } else if (IROp->Op == OP_CODEBLOCK) {
      auto CodeBlock = IROp->C<IROp_CodeBlock>();

      Node = CodeBlock->Begin;
    } else {
      Node = RealNode->Next;
    }

    return *this;
  }

  AllNodesIterator operator--() {
    auto IROp = Node.GetNode(BaseList)->Op(IRList);

    if (IROp->Op == OP_BEGINBLOCK) {
      auto BeginBlock = IROp->C<IROp_EndBlock>();

      Node = BeginBlock->BlockHeader;
    } else if (IROp->Op == OP_CODEBLOCK) {
      auto PrevBlockWrapper = Node.GetNode(BaseList)->Header.Previous;
      auto PrevCodeBlock = PrevBlockWrapper.GetNode(BaseList)->Op(IRList)->C<IROp_CodeBlock>();

      Node = PrevCodeBlock->Last;
    } else {
      Node = Node.GetNode(BaseList)->Header.Previous;
    }

    return *this;
  }

  [[nodiscard]]
  static AllNodesIterator Invalid() {
    return AllNodesIterator(0, 0);
  }
};

class IRListView;
class IREmitter;

template<typename Type>
inline NodeID NodeWrapperBase<Type>::ID() const {
  return NodeID(NodeOffset / sizeof(IR::OrderedNode));
}

[[nodiscard]]
bool IsBlockExit(FEXCore::IR::IROps Op);

void Dump(fextl::stringstream* out, const IRListView* IR);
} // namespace FEXCore::IR

template<>
struct std::hash<FEXCore::IR::NodeID> {
  size_t operator()(const FEXCore::IR::NodeID& ID) const noexcept {
    return std::hash<FEXCore::IR::NodeID::value_type> {}(ID.Value);
  }
};

template<>
struct fmt::formatter<FEXCore::IR::NodeID> : fmt::formatter<FEXCore::IR::NodeID::value_type> {
  using Base = fmt::formatter<FEXCore::IR::NodeID::value_type>;

  // Pass-through the underlying value, so IDs can
  // be formatted like any integral value.
  template<typename FormatContext>
  auto format(const FEXCore::IR::NodeID& ID, FormatContext& ctx) const {
    return Base::format(ID.Value, ctx);
  }
};

template<>
struct fmt::formatter<FEXCore::IR::RegisterClassType> : fmt::formatter<FEXCore::IR::RegisterClassType::value_type> {
  using Base = fmt::formatter<FEXCore::IR::RegisterClassType::value_type>;

  template<typename FormatContext>
  auto format(const FEXCore::IR::RegisterClassType& Class, FormatContext& ctx) const {
    return Base::format(Class.Val, ctx);
  }
};

template<>
struct fmt::formatter<FEXCore::IR::FenceType> : fmt::formatter<std::underlying_type_t<FEXCore::IR::FenceType>> {
  using Base = fmt::formatter<std::underlying_type_t<FEXCore::IR::FenceType>>;

  template<typename FormatContext>
  auto format(const FEXCore::IR::FenceType& Fence, FormatContext& ctx) const {
    return Base::format(FEXCore::ToUnderlying(Fence), ctx);
  }
};

template<>
struct fmt::formatter<FEXCore::IR::MemOffsetType> : fmt::formatter<std::underlying_type_t<FEXCore::IR::MemOffsetType>> {
  using Base = fmt::formatter<std::underlying_type_t<FEXCore::IR::MemOffsetType>>;

  template<typename FormatContext>
  auto format(const FEXCore::IR::MemOffsetType& Type, FormatContext& ctx) const {
    return Base::format(FEXCore::ToUnderlying(Type), ctx);
  }
};

template<>
struct fmt::formatter<FEXCore::IR::OpSize> : fmt::formatter<std::underlying_type_t<FEXCore::IR::OpSize>> {
  using Base = fmt::formatter<std::underlying_type_t<FEXCore::IR::OpSize>>;

  template<typename FormatContext>
  auto format(const FEXCore::IR::OpSize& OpSize, FormatContext& ctx) const {
    return Base::format(FEXCore::ToUnderlying(OpSize), ctx);
  }
};
