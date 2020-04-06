#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <sstream>

namespace FEXCore::IR {

/**
 * @brief The IROp_Header is an dynamically sized array
 * At the end it contains a uint8_t for the number of arguments that Op has
 * Then there is an unsized array of NodeWrapper arguments for the number of arguments this op has
 * The op structures that are including the header must ensure that they pad themselves correctly to the number of arguments used
 */
struct IROp_Header;

class OrderedNode;
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
struct NodeWrapperBase final {
  // On x86-64 using a uint64_t type is more efficient since RIP addressing gives you [<Base> + <Index> + <imm offset>]
  // On AArch64 using uint32_t is just more memory efficient. 32bit or 64bit offset doesn't matter
  // We use uint32_t to be more memory efficient (Cuts our node list size in half)
  using NodeOffsetType = uint32_t;
  NodeOffsetType NodeOffset;

  static NodeWrapperBase WrapOffset(NodeOffsetType Offset) {
    NodeWrapperBase Wrapped;
    Wrapped.NodeOffset = Offset;
    return Wrapped;
  }

  static NodeWrapperBase WrapPtr(uintptr_t Base, uintptr_t Value) {
    NodeWrapperBase Wrapped;
    Wrapped.SetOffset(Base, Value);
    return Wrapped;
  }

  static void *UnwrapNode(uintptr_t Base, NodeWrapperBase Node) {
    return Node.GetNode(Base);
  }

  uint32_t ID() const;

  bool IsInvalid() const { return NodeOffset == 0; }

  explicit NodeWrapperBase() = default;

  Type *GetNode(uintptr_t Base) { return reinterpret_cast<Type*>(Base + NodeOffset); }
  Type const *GetNode(uintptr_t Base) const { return reinterpret_cast<Type*>(Base + NodeOffset); }

  void SetOffset(uintptr_t Base, uintptr_t Value) { NodeOffset = Value - Base; }
  bool operator==(NodeWrapperBase const &rhs) { return NodeOffset == rhs.NodeOffset; }
};

static_assert(std::is_pod<NodeWrapperBase<OrderedNode>>::value);
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
* @brief This is our NodeWrapperIterator
* This stores both the memory base and the provided NodeWrapper to be able to walk the list of nodes directly
* Only the increment and decrement implementations of this class require understanding the implementation details of OrderedNode
*/
class NodeWrapperIterator final {
public:
	using value_type              = OrderedNodeWrapper;
	using size_type               = std::size_t;
	using difference_type         = std::ptrdiff_t;
	using reference               = value_type&;
	using const_reference         = const value_type&;
	using pointer                 = value_type*;
	using const_pointer           = const value_type*;
	using iterator                = NodeWrapperIterator;
	using const_iterator          = const NodeWrapperIterator;
	using reverse_iterator        = iterator;
	using const_reverse_iterator  = const_iterator;
	using iterator_category       = std::bidirectional_iterator_tag;

	using NodeType = value_type;
	using NodePtr = value_type*;
	using NodeRef = value_type&;

	NodeWrapperIterator(uintptr_t Base) : BaseList {Base} {}
	explicit NodeWrapperIterator(uintptr_t Base, NodeType Ptr) : BaseList {Base}, Node {Ptr} {}

	bool operator==(const NodeWrapperIterator &rhs) const {
		return Node.NodeOffset == rhs.Node.NodeOffset;
	}

	bool operator!=(const NodeWrapperIterator &rhs) const {
		return !operator==(rhs);
	}

	NodeWrapperIterator operator++() {
		OrderedNodeHeader *RealNode = reinterpret_cast<OrderedNodeHeader*>(Node.GetNode(BaseList));
		Node = RealNode->Next;
		return *this;
	}

	NodeWrapperIterator operator--() {
		OrderedNodeHeader *RealNode = reinterpret_cast<OrderedNodeHeader*>(Node.GetNode(BaseList));
		Node = RealNode->Previous;
		return *this;
	}

	NodeRef operator*() {
		return Node;
	}

	NodePtr operator()() {
		return &Node;
	}

  static NodeWrapperIterator Invalid() {
    return NodeWrapperIterator(0);
  }

private:
	uintptr_t BaseList{};
	NodeType Node{};
};

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
  friend class NodeWrapperIterator;
  friend class OrderedList;
  public:
    // These three values are laid out very specifically to make it fast to access the NodeWrappers specifically
    OrderedNodeHeader Header;
    uint32_t NumUses;

    using value_type              = OrderedNodeWrapper;

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

    OrderedNode *append(uintptr_t Base, OrderedNode *Node) {
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

    OrderedNode *prepend(uintptr_t Base, OrderedNode *Node) {
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
    size_t size(uintptr_t Base) const {
      size_t Size = 1;
      // Walk the list forward until we hit a sentinal
      value_type Current = Header.Next;
      while (Current.NodeOffset != 0) {
        ++Size;
        OrderedNode *RealNode = Current.GetNode(Base);
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

    IROp_Header const* Op(uintptr_t Base) const { return Header.Value.GetNode(Base); }
    IROp_Header *Op(uintptr_t Base) { return Header.Value.GetNode(Base); }

    uint32_t GetUses() const { return NumUses; }

    void AddUse() { ++NumUses; }
    void RemoveUse() { --NumUses; }

    value_type Wrapped(uintptr_t Base) {
      value_type Tmp;
      Tmp.SetOffset(Base, reinterpret_cast<uintptr_t>(this));
      return Tmp;
    }

  private:
    value_type WrappedOffset(uint32_t Offset) {
      value_type Tmp;
      Tmp.NodeOffset = Offset;
      return Tmp;
    }

    static void SetPrevious(uintptr_t Base, value_type Node, value_type New) {
      if (Node.NodeOffset == 0) return;
      OrderedNode *RealNode = Node.GetNode(Base);
      RealNode->Header.Previous = New;
    }

    static void SetNext(uintptr_t Base, value_type Node, value_type New) {
      if (Node.NodeOffset == 0) return;
      OrderedNode *RealNode = Node.GetNode(Base);
      RealNode->Header.Next = New;
    }

    void SetUses(uint32_t Uses) { NumUses = Uses; }
};

static_assert(std::is_pod<OrderedNode>::value);
static_assert(std::is_trivially_copyable<OrderedNode>::value);
static_assert(offsetof(OrderedNode, Header) == 0);
static_assert(sizeof(OrderedNode) == (sizeof(OrderedNodeHeader) + sizeof(uint32_t)));

struct RegisterClassType final {
  uint32_t Val;
  operator uint32_t() {
    return Val;
  }
};

struct CondClassType final {
  uint8_t Val;
  operator uint8_t() {
    return Val;
  }
};

#define IROP_ENUM
#define IROP_STRUCTS
#define IROP_SIZES
#include <FEXCore/IR/IRDefines.inc>

template<bool>
class IRListView;

void Dump(std::stringstream *out, IRListView<false> const* IR);

template<typename Type>
inline uint32_t NodeWrapperBase<Type>::ID() const { return NodeOffset / sizeof(IR::OrderedNode); }

};
