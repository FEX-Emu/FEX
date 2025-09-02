// SPDX-License-Identifier: MIT
#pragma once

#include "Interface/IR/IR.h"

#include <FEXCore/Core/Context.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/ThreadPoolAllocator.h>
#include <FEXCore/fextl/vector.h>

#include <cstddef>
#include <cstring>
#include <tuple>
#include <istream>
#include <ostream>

namespace FEXCore::IR {
/**
 * @brief This is purely an intrusive allocator
 * This doesn't support any form of ordering at all
 * Just provides a chunk of memory for allocating IR nodes from
 *
 * Can potentially support reallocation if we are smart and make sure to invalidate anything holding a true pointer
 */
class DualIntrusiveAllocator {
public:
  [[nodiscard]]
  bool DataCheckSize(size_t Size) const {
    size_t NewOffset = DataCurrentOffset + Size;
    return NewOffset <= MemorySize;
  }

  [[nodiscard]]
  bool ListCheckSize(size_t Size) const {
    size_t NewOffset = ListCurrentOffset + Size;
    return NewOffset <= MemorySize;
  }

  [[nodiscard]]
  void* DataAllocate(size_t Size) {
    LOGMAN_THROW_A_FMT(DataCheckSize(Size), "Ran out of space in DualIntrusiveAllocator during allocation");
    size_t NewOffset = DataCurrentOffset + Size;
    uintptr_t NewPointer = Data + DataCurrentOffset;
    DataCurrentOffset = NewOffset;
    return reinterpret_cast<void*>(NewPointer);
  }

  [[nodiscard]]
  void* ListAllocate(size_t Size) {
    LOGMAN_THROW_A_FMT(ListCheckSize(Size), "Ran out of space in DualIntrusiveAllocator during allocation");
    size_t NewOffset = ListCurrentOffset + Size;
    uintptr_t NewPointer = List + ListCurrentOffset;
    ListCurrentOffset = NewOffset;
    return reinterpret_cast<void*>(NewPointer);
  }

  [[nodiscard]]
  size_t DataSize() const {
    return DataCurrentOffset;
  }
  [[nodiscard]]
  size_t DataBackingSize() const {
    return MemorySize;
  }

  [[nodiscard]]
  size_t ListSize() const {
    return ListCurrentOffset;
  }
  [[nodiscard]]
  size_t ListBackingSize() const {
    return MemorySize;
  }

  [[nodiscard]]
  uintptr_t DataBegin() const {
    return Data;
  }
  [[nodiscard]]
  uintptr_t ListBegin() const {
    return List;
  }

  void Reset() {
    DataCurrentOffset = 0;
    ListCurrentOffset = 0;
  }

  void CopyData(const DualIntrusiveAllocator& rhs) {
    DataCurrentOffset = rhs.DataCurrentOffset;
    ListCurrentOffset = rhs.ListCurrentOffset;
    memcpy(reinterpret_cast<void*>(Data), reinterpret_cast<void*>(rhs.Data), DataCurrentOffset);
    memcpy(reinterpret_cast<void*>(List), reinterpret_cast<void*>(rhs.List), ListCurrentOffset);
  }

protected:
  DualIntrusiveAllocator(size_t Size)
    : MemorySize {Size} {}

  uintptr_t Data {};
  uintptr_t List {};
  size_t DataCurrentOffset {0};
  size_t ListCurrentOffset {0};
  size_t MemorySize {};
};

class DualIntrusiveAllocatorMalloc final : public DualIntrusiveAllocator {
public:
  DualIntrusiveAllocatorMalloc(size_t Size)
    : DualIntrusiveAllocator {Size} {
    Data = reinterpret_cast<uintptr_t>(FEXCore::Allocator::malloc(Size * 2));
    List = reinterpret_cast<uintptr_t>(Data + Size);
  }

  ~DualIntrusiveAllocatorMalloc() {
    FEXCore::Allocator::free(reinterpret_cast<void*>(Data));
  }
};

class DualIntrusiveAllocatorThreadPool final : public DualIntrusiveAllocator {
public:
  DualIntrusiveAllocatorThreadPool(FEXCore::Utils::IntrusivePooledAllocator& ThreadAllocator, size_t Size)
    : DualIntrusiveAllocator {Size}
    , PoolObject {ThreadAllocator, Size * 2} {
    // Claim a buffer on allocation
    PoolObject.ReownOrClaimBuffer();
  }

  void ReownOrClaimBuffer() {
    Data = PoolObject.ReownOrClaimBuffer();
    List = Data + MemorySize;
  }

  void DelayedDisownBuffer() {
    PoolObject.DelayedDisownBuffer();
  }

private:
  Utils::PoolBufferWithTimedRetirement<uintptr_t, 5000, 500> PoolObject;
};

class IRListView final {
public:
  IRListView() = delete;

  IRListView(DualIntrusiveAllocator* Data)
    : IRListView(reinterpret_cast<void*>(Data->DataBegin()), reinterpret_cast<void*>(Data->ListBegin()), Data->DataSize(), Data->ListSize()) {}

  IRListView(IRListView* Old)
    : IRListView(Old->IRDataInternal, Old->ListDataInternal, Old->DataSize, Old->ListSize) {}

  IRListView(void* IRData_, void* ListData_, size_t DataSize_, size_t ListSize_)
    : IRDataInternal(IRData_)
    , ListDataInternal(ListData_)
    , DataSize(DataSize_)
    , ListSize(ListSize_) {}

  [[nodiscard]]
  size_t GetInlineSize() const {
    static_assert(sizeof(*this) == 32);
    return sizeof(*this) + DataSize + ListSize;
  }

  [[nodiscard]]
  size_t GetDataSize() const {
    return DataSize;
  }
  [[nodiscard]]
  size_t GetListSize() const {
    return ListSize;
  }
  [[nodiscard]]
  size_t GetSSACount() const {
    return ListSize / sizeof(OrderedNode);
  }

  [[nodiscard]]
  NodeID GetID(const Ref Node) const {
    return Node->Wrapped(GetListData()).ID();
  }

  [[nodiscard]]
  Ref GetHeaderNode() const {
    OrderedNodeWrapper Wrapped;
    Wrapped.NodeOffset = sizeof(OrderedNode);
    return Wrapped.GetNode(GetListData());
  }

  [[nodiscard]]
  IROp_IRHeader* GetHeader() const {
    return GetOp<IROp_IRHeader>(GetHeaderNode());
  }

  [[nodiscard]]
  unsigned PostRA() const {
    return GetHeader()->PostRA;
  }

  [[nodiscard]]
  unsigned SpillSlots() const {
    return GetHeader()->SpillSlots;
  }

  template<typename T>
  [[nodiscard]]
  T* GetOp(Ref Node) const {
    auto OpHeader = Node->Op(GetData());
    auto Op = OpHeader->template CW<T>();

    // If we are casting to something narrower than just the header, check the opcode.
    if constexpr (!std::is_same<T, IROp_Header>::value) {
      LOGMAN_THROW_A_FMT(Op->OPCODE == Op->Header.Op, "Expected Node to be '{}'. Found '{}' instead", GetName(Op->OPCODE),
                         GetName(Op->Header.Op));
    }

    return Op;
  }

  template<typename T>
  [[nodiscard]]
  T* GetOp(OrderedNodeWrapper Wrapper) const {
    auto Node = Wrapper.GetNode(GetListData());
    return GetOp<T>(Node);
  }

  [[nodiscard]]
  Ref GetNode(OrderedNodeWrapper Wrapper) const {
    return Wrapper.GetNode(GetListData());
  }

  ///< Gets an OrderedNode from the IRListView as an OrderedNodeWrapper.
  [[nodiscard]]
  OrderedNodeWrapper WrapNode(Ref Node) const {
    return Node->Wrapped(GetListData());
  }

private:
  struct BlockRange {
    using iterator = NodeIterator;
    const IRListView* View;

    BlockRange(const IRListView* parent)
      : View(parent) {};

    [[nodiscard]]
    iterator begin() const noexcept {
      auto Header = View->GetHeader();
      return iterator(View->GetListData(), View->GetData(), Header->Blocks);
    }

    [[nodiscard]]
    iterator end() const noexcept {
      return iterator(View->GetListData(), View->GetData());
    }
  };

  struct CodeRange {
    using iterator = NodeIterator;
    const IRListView* View;
    const OrderedNodeWrapper BlockWrapper;

    CodeRange(const IRListView* parent, OrderedNodeWrapper block)
      : View(parent)
      , BlockWrapper(block) {};

    [[nodiscard]]
    iterator begin() const noexcept {
      auto Block = View->GetOp<IROp_CodeBlock>(BlockWrapper);
      return iterator(View->GetListData(), View->GetData(), Block->Begin);
    }

    [[nodiscard]]
    iterator end() const noexcept {
      return iterator(View->GetListData(), View->GetData());
    }
  };

  struct AllCodeRange {
    using iterator = AllNodesIterator; // Diffrent Iterator
    const IRListView* View;

    AllCodeRange(const IRListView* parent)
      : View(parent) {};

    [[nodiscard]]
    iterator begin() const noexcept {
      auto Header = View->GetHeader();
      return iterator(View->GetListData(), View->GetData(), Header->Blocks);
    }

    [[nodiscard]]
    iterator end() const noexcept {
      return iterator(View->GetListData(), View->GetData());
    }
  };

public:
  using iterator = NodeIterator;

  [[nodiscard]]
  BlockRange GetBlocks() const {
    return BlockRange(this);
  }

  [[nodiscard]]
  CodeRange GetCode(const Ref block) const {
    return CodeRange(this, block->Wrapped(GetListData()));
  }

  [[nodiscard]]
  AllCodeRange GetAllCode() const {
    return AllCodeRange(this);
  }

  [[nodiscard]]
  iterator begin() const noexcept {
    OrderedNodeWrapper Wrapped;
    Wrapped.NodeOffset = sizeof(OrderedNode);
    return iterator(GetListData(), GetData(), Wrapped);
  }

  /**
   * @brief This is not an iterator that you can reverse iterator through!
   *
   * @return Our iterator sentinel to ensure ending correctly
   */
  [[nodiscard]]
  iterator end() const noexcept {
    OrderedNodeWrapper Wrapped;
    Wrapped.NodeOffset = 0;
    return iterator(GetListData(), GetData(), Wrapped);
  }

  /**
   * @brief Convert a OrderedNodeWrapper to an interator that we can iterate over
   * @return Iterator for this op
   */
  [[nodiscard]]
  iterator at(OrderedNodeWrapper Wrapped) const noexcept {
    return iterator(GetListData(), GetData(), Wrapped);
  }

  [[nodiscard]]
  iterator at(NodeID ID) const noexcept {
    OrderedNodeWrapper Wrapped;
    Wrapped.NodeOffset = ID.Value * sizeof(OrderedNode);
    return iterator(GetListData(), GetData(), Wrapped);
  }

  [[nodiscard]]
  iterator at(const Ref Node) const noexcept {
    const auto ListData = GetListData();
    auto Wrapped = Node->Wrapped(ListData);
    return iterator(ListData, GetData(), Wrapped);
  }

  [[nodiscard]]
  uintptr_t GetData() const {
    return reinterpret_cast<uintptr_t>(IRDataInternal ? IRDataInternal : InlineData);
  }

  [[nodiscard]]
  uintptr_t GetListData() const {
    return reinterpret_cast<uintptr_t>(ListDataInternal ? ListDataInternal : &InlineData[DataSize]);
  }

private:
  void* IRDataInternal;
  void* ListDataInternal;
  size_t DataSize;
  size_t ListSize;
  uint8_t InlineData[0];
};

} // namespace FEXCore::IR
