#pragma once

#include "FEXCore/IR/IR.h"
#include <FEXCore/Utils/LogManager.h>

#include <cassert>
#include <cstddef>
#include <cstring>
#include <tuple>
#include <vector>
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
class IntrusiveAllocator final {
  public:
    IntrusiveAllocator() = delete;
    IntrusiveAllocator(IntrusiveAllocator &&) = delete;
    IntrusiveAllocator(size_t Size)
      : MemorySize {Size} {
      Data = reinterpret_cast<uintptr_t>(malloc(Size));
    }

    ~IntrusiveAllocator() {
      free(reinterpret_cast<void*>(Data));
    }

    bool CheckSize(size_t Size) {
      size_t NewOffset = CurrentOffset + Size;
      return NewOffset <= MemorySize;
    }

    void *Allocate(size_t Size) {
      assert(CheckSize(Size) &&
        "Ran out of space in IntrusiveAllocator during allocation");
      size_t NewOffset = CurrentOffset + Size;
      uintptr_t NewPointer = Data + CurrentOffset;
      CurrentOffset = NewOffset;
      return reinterpret_cast<void*>(NewPointer);
    }

    size_t Size() const { return CurrentOffset; }
    size_t BackingSize() const { return MemorySize; }

    uintptr_t const Begin() const { return Data; }

    void Reset() { CurrentOffset = 0; }

    void CopyData(IntrusiveAllocator const &rhs) {
      CurrentOffset = rhs.CurrentOffset;
      memcpy(reinterpret_cast<void*>(Data), reinterpret_cast<void*>(rhs.Data), CurrentOffset);
    }

  private:
    size_t CurrentOffset {0};
    size_t MemorySize;
    uintptr_t Data;
};

class IRListView final {
public:
  IRListView() = delete;
  IRListView(IRListView &&) = delete;

  IRListView(IntrusiveAllocator *Data, IntrusiveAllocator *List, bool _IsCopy) : IsCopy(_IsCopy) {
    DataSize = Data->Size();
    ListSize = List->Size();

    if (IsCopy) {
      IRData = malloc(DataSize + ListSize);
      ListData = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(IRData) + DataSize);
      memcpy(IRData, reinterpret_cast<void*>(Data->Begin()), DataSize);
      memcpy(ListData, reinterpret_cast<void*>(List->Begin()), ListSize);
    }
    else {
      // We are just pointing to the data
      IRData = reinterpret_cast<void*>(Data->Begin());
      ListData = reinterpret_cast<void*>(List->Begin());
    }
  }

  IRListView(IRListView *Old, bool _IsCopy) : IsCopy(_IsCopy) {
    DataSize = Old->DataSize;
    ListSize = Old->ListSize;
    if (IsCopy) {
      IRData = malloc(DataSize + ListSize);
      ListData = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(IRData) + DataSize);
      memcpy(IRData, Old->IRData, DataSize);
      memcpy(ListData, Old->ListData, ListSize);
    } else {
      IRData = Old->IRData;
      ListData = Old->ListData;
    }
  }

  IRListView(std::istream& stream) : IsCopy(true) {
    stream.read((char*)&DataSize, sizeof(DataSize));
    stream.read((char*)&ListSize, sizeof(ListSize));
    
    IRData = malloc(DataSize + ListSize);
    ListData = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(IRData) + DataSize);
    stream.read((char*)IRData, DataSize);
    stream.read((char*)ListData, ListSize);
  }

  ~IRListView() {
    if (IsCopy) {
      free (IRData);
      // ListData is just offset from IRData
    }
  }

  void Serialize(std::ostream& stream) {
    stream.write((char*)&DataSize, sizeof(DataSize));
    stream.write((char*)&ListSize, sizeof(ListSize));
    stream.write((char*)IRData, DataSize);
    stream.write((char*)ListData, ListSize);
  }

  IRListView *CreateCopy() {
    return new IRListView(this, true);
  }

  uintptr_t const GetData() const { return reinterpret_cast<uintptr_t>(IRData); }
  uintptr_t const GetListData() const { return reinterpret_cast<uintptr_t>(ListData); }

  size_t GetDataSize() const { return DataSize; }
  size_t GetListSize() const { return ListSize; }
  size_t GetSSACount() const { return ListSize / sizeof(OrderedNode); }

  uint32_t GetID(OrderedNode *Node) const {
    return Node->Wrapped(GetListData()).ID();
  }

  OrderedNode* GetHeaderNode() const {
    OrderedNodeWrapper Wrapped;
    Wrapped.NodeOffset = sizeof(OrderedNode);
    return Wrapped.GetNode(GetListData());
  }

  IROp_IRHeader *GetHeader() const {
    return GetOp<IROp_IRHeader>(GetHeaderNode());
  }

  template <typename T>
  T *GetOp(OrderedNode *Node) const {
    auto OpHeader = Node->Op(GetData());
    auto Op = OpHeader->template CW<T>();

    // If we are casting to something narrower than just the header, check the opcode.
    if constexpr (!std::is_same<T, IROp_Header>::value) {
      LogMan::Throw::A(Op->OPCODE == Op->Header.Op, "Expected Node to be '%s'. Found '%s' instead", GetName(Op->OPCODE), GetName(Op->Header.Op));
    }

    return Op;
  }

  template <typename T>
  T *GetOp(OrderedNodeWrapper Wrapper) const {
    auto Node = Wrapper.GetNode(GetListData());
    return GetOp<T>(Node);
  }

  OrderedNode* GetNode(OrderedNodeWrapper Wrapper) const {
    return Wrapper.GetNode(GetListData());
  }

private:
  struct BlockRange {
    using iterator = NodeIterator;
    const IRListView *View;

    BlockRange(const IRListView *parent) : View(parent) {};

    iterator begin() const noexcept {
      auto Header = View->GetHeader();
      return iterator(View->GetListData(), View->GetData(), Header->Blocks);
    }

    iterator end() const noexcept {
      return iterator(View->GetListData(), View->GetData());
    }
  };

  struct CodeRange {
    using iterator = NodeIterator;
    const IRListView *View;
    const OrderedNodeWrapper BlockWrapper;

    CodeRange(const IRListView *parent, OrderedNodeWrapper block) : View(parent), BlockWrapper(block) {};

    iterator begin() const noexcept {
      auto Block = View->GetOp<IROp_CodeBlock>(BlockWrapper);
      return iterator(View->GetListData(), View->GetData(), Block->Begin);
    }

    iterator end() const noexcept {
      return iterator(View->GetListData(), View->GetData());
    }
  };

  struct AllCodeRange {
    using iterator = AllNodesIterator; // Diffrent Iterator
    const IRListView *View;

    AllCodeRange(const IRListView *parent) : View(parent) {};

    iterator begin() const noexcept {
      auto Header = View->GetHeader();
      return iterator(View->GetListData(), View->GetData(), Header->Blocks);
    }

    iterator end() const noexcept {
      return iterator(View->GetListData(), View->GetData());
    }
  };


public:

  BlockRange GetBlocks() const {
    return BlockRange(this);
  }

  CodeRange GetCode(OrderedNode *block) const {
    return CodeRange(this, block->Wrapped(GetListData()));
  }

  AllCodeRange GetAllCode() const {
    return AllCodeRange(this);
  }

  using iterator = NodeIterator;

  iterator begin() const noexcept
  {
    OrderedNodeWrapper Wrapped;
    Wrapped.NodeOffset = sizeof(OrderedNode);
    return iterator(reinterpret_cast<uintptr_t>(ListData), reinterpret_cast<uintptr_t>(IRData), Wrapped);
  }

  /**
   * @brief This is not an iterator that you can reverse iterator through!
   *
   * @return Our iterator sentinal to ensure ending correctly
   */
  iterator end() const noexcept
  {
    OrderedNodeWrapper Wrapped;
    Wrapped.NodeOffset = 0;
    return iterator(reinterpret_cast<uintptr_t>(ListData), reinterpret_cast<uintptr_t>(IRData), Wrapped);
  }

  /**
   * @brief Convert a OrderedNodeWrapper to an interator that we can iterate over
   * @return Iterator for this op
   */
  iterator at(OrderedNodeWrapper Wrapped) const noexcept {
    return iterator(reinterpret_cast<uintptr_t>(ListData), reinterpret_cast<uintptr_t>(IRData), Wrapped);
  }

  iterator at(uint32_t ID) const noexcept {
    OrderedNodeWrapper Wrapped;
    Wrapped.NodeOffset = ID * sizeof(OrderedNode);
    return iterator(reinterpret_cast<uintptr_t>(ListData), reinterpret_cast<uintptr_t>(IRData), Wrapped);
  }

  iterator at(OrderedNode *Node) const noexcept {
    auto Wrapped = Node->Wrapped(reinterpret_cast<uintptr_t>(ListData));
    return iterator(reinterpret_cast<uintptr_t>(ListData), reinterpret_cast<uintptr_t>(IRData), Wrapped);
  }

private:
  void *IRData;
  void *ListData;
  size_t DataSize;
  size_t ListSize;
  bool IsCopy;
};
}

