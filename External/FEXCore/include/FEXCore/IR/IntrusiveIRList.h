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
class DualIntrusiveAllocator final {
  public:
    DualIntrusiveAllocator() = delete;
    DualIntrusiveAllocator(DualIntrusiveAllocator &&) = delete;
    DualIntrusiveAllocator(size_t Size)
      : MemorySize {Size} {
      Data = reinterpret_cast<uintptr_t>(malloc(Size * 2));
      List = reinterpret_cast<uintptr_t>(Data + Size);
    }

    ~DualIntrusiveAllocator() {
      free(reinterpret_cast<void*>(Data));
    }

    bool DataCheckSize(size_t Size) {
      size_t NewOffset = DataCurrentOffset + Size;
      return NewOffset <= MemorySize;
    }

    bool ListCheckSize(size_t Size) {
      size_t NewOffset = ListCurrentOffset + Size;
      return NewOffset <= MemorySize;
    }

    void *DataAllocate(size_t Size) {
      assert(DataCheckSize(Size) &&
        "Ran out of space in DualIntrusiveAllocator during allocation");
      size_t NewOffset = DataCurrentOffset + Size;
      uintptr_t NewPointer = Data + DataCurrentOffset;
      DataCurrentOffset = NewOffset;
      return reinterpret_cast<void*>(NewPointer);
    }

    void *ListAllocate(size_t Size) {
      assert(ListCheckSize(Size) &&
        "Ran out of space in DualIntrusiveAllocator during allocation");
      size_t NewOffset = ListCurrentOffset + Size;
      uintptr_t NewPointer = List + ListCurrentOffset;
      ListCurrentOffset = NewOffset;
      return reinterpret_cast<void*>(NewPointer);
    }

    size_t DataSize() const { return DataCurrentOffset; }
    size_t DataBackingSize() const { return MemorySize; }

    size_t ListSize() const { return ListCurrentOffset; }
    size_t ListBackingSize() const { return MemorySize; }

    uintptr_t const DataBegin() const { return Data; }
    uintptr_t const ListBegin() const { return List; }

    void Reset() { DataCurrentOffset = 0; ListCurrentOffset = 0; }

    void CopyData(DualIntrusiveAllocator const &rhs) {
      DataCurrentOffset = rhs.DataCurrentOffset;
      ListCurrentOffset = rhs.ListCurrentOffset;
      memcpy(reinterpret_cast<void*>(Data), reinterpret_cast<void*>(rhs.Data), DataCurrentOffset);
      memcpy(reinterpret_cast<void*>(List), reinterpret_cast<void*>(rhs.List), ListCurrentOffset);
    }

  private:
    uintptr_t Data;
    uintptr_t List;
    size_t DataCurrentOffset {0};
    size_t ListCurrentOffset {0};
    size_t MemorySize;
};


class IRListView final {
public:
  IRListView() = delete;
  IRListView(IRListView &&) = delete;

  IRListView(DualIntrusiveAllocator *Data, bool _IsCopy) : IsCopy(_IsCopy) {
    DataSize = Data->DataSize();
    ListSize = Data->ListSize();

    if (IsCopy) {
      IRData = malloc(DataSize + ListSize);
      ListData = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(IRData) + DataSize);
      memcpy(IRData, reinterpret_cast<void*>(Data->DataBegin()), DataSize);
      memcpy(ListData, reinterpret_cast<void*>(Data->ListBegin()), ListSize);
    }
    else {
      // We are just pointing to the data
      IRData = reinterpret_cast<void*>(Data->DataBegin());
      ListData = reinterpret_cast<void*>(Data->ListBegin());
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
  bool IsShared() const { return _IsShared; }
  void SetShared(bool Set) { _IsShared = Set; }

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
      LOGMAN_THROW_A(Op->OPCODE == Op->Header.Op, "Expected Node to be '%s'. Found '%s' instead", GetName(Op->OPCODE), GetName(Op->Header.Op));
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
  bool _IsShared {false};
};

struct IRListViewDeleter {
  void operator()(IRListView* r) {
    if (!r->IsShared()) {
      delete r;
    }
  }
};
}

