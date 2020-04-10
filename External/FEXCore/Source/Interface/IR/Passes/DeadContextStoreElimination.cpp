#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include <FEXCore/Core/CoreState.h>

namespace {
  struct ContextMemberClassification {
    size_t Offset;
    uint8_t Size;
  };

  enum LastAccessType {
    ACCESS_NONE,          ///< Was never previously accessed
    ACCESS_WRITE,         ///< Was fully overwritten
    ACCESS_READ,          ///< Was fully read
    ACCESS_PARTIAL_WRITE, ///< Was partially written
    ACCESS_PARTIAL_READ,  ///< Was partially read
    ACCESS_INVALID,       ///< Accessing this is invalid
  };

  struct ContextMemberInfo {
    ContextMemberClassification Class;
    LastAccessType Accessed;
    uint8_t AccessRegClass;
    uint32_t AccessOffset;
    uint8_t AccessSize;
    FEXCore::IR::OrderedNode *Node;
    FEXCore::IR::OrderedNode *Node2;
  };

  using ContextInfo = std::vector<ContextMemberInfo>;

  constexpr static std::array<LastAccessType, 8> DefaultAccess = {
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_INVALID, // PAD
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
  };

  static void ClassifyContextStruct(ContextInfo *ContextClassification) {
    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, rip),
        sizeof(FEXCore::Core::CPUState::rip),
      },
      DefaultAccess[0],
      255,
    });

    for (size_t i = 0; i < 16; ++i) {
      ContextClassification->emplace_back(ContextMemberInfo{
        ContextMemberClassification {
          offsetof(FEXCore::Core::CPUState, gregs[0]) + sizeof(FEXCore::Core::CPUState::gregs[0]) * i,
          sizeof(FEXCore::Core::CPUState::gregs[0]),
        },
        DefaultAccess[1],
        255,
      });
    }

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, gregs[16]),
        sizeof(uint64_t),
      },
      DefaultAccess[2], ///< NOP padding
      255,
    });

    for (size_t i = 0; i < 16; ++i) {
      ContextClassification->emplace_back(ContextMemberInfo{
        ContextMemberClassification {
          offsetof(FEXCore::Core::CPUState, xmm[0][0]) + sizeof(FEXCore::Core::CPUState::xmm[0]) * i,
          sizeof(FEXCore::Core::CPUState::xmm[0]),
        },
        DefaultAccess[3],
        255,
      });
    }

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, gs),
        sizeof(FEXCore::Core::CPUState::gs),
      },
      DefaultAccess[4],
      255,
    });

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, fs),
        sizeof(FEXCore::Core::CPUState::fs),
      },
      DefaultAccess[5],
      255,
    });

    for (size_t i = 0; i < (sizeof(FEXCore::Core::CPUState::flags) / sizeof(FEXCore::Core::CPUState::flags[0])); ++i) {
      ContextClassification->emplace_back(ContextMemberInfo{
        ContextMemberClassification {
          offsetof(FEXCore::Core::CPUState, flags[0]) + sizeof(FEXCore::Core::CPUState::flags[0]) * i,
          sizeof(FEXCore::Core::CPUState::flags[0]),
        },
        DefaultAccess[6],
        255,
      });
    }

    for (size_t i = 0; i < 8; ++i) {
      ContextClassification->emplace_back(ContextMemberInfo{
        ContextMemberClassification {
          offsetof(FEXCore::Core::CPUState, mm[0][0]) + sizeof(FEXCore::Core::CPUState::mm[0]) * i,
          sizeof(FEXCore::Core::CPUState::mm[0]),
        },
        DefaultAccess[7],
        255,
      });
    }

    size_t ClassifiedStructSize{};
    for (auto &it : *ContextClassification) {
      ClassifiedStructSize += it.Class.Size;
    }

    LogMan::Throw::A(ClassifiedStructSize == sizeof(FEXCore::Core::CPUState),
      "Classified CPUStruct size doesn't match real CPUState struct size! %ld != %ld",
      ClassifiedStructSize, sizeof(FEXCore::Core::CPUState));
  }

  static void ResetClassificationAccesses(ContextInfo *ContextClassification) {
    auto SetAccess = [&](size_t Offset, auto Access) {
      ContextClassification->at(Offset).Accessed = Access;
      ContextClassification->at(Offset).AccessRegClass = 255;
      ContextClassification->at(Offset).AccessOffset = 0;
    };
    size_t Offset = 0;
    SetAccess(Offset++, DefaultAccess[0]);
    for (size_t i = 0; i < 16; ++i) {
      SetAccess(Offset++, DefaultAccess[1]);
    }
    SetAccess(Offset++, DefaultAccess[2]);

    for (size_t i = 0; i < 16; ++i) {
      SetAccess(Offset++, DefaultAccess[3]);
    }

    SetAccess(Offset++, DefaultAccess[4]);
    SetAccess(Offset++, DefaultAccess[5]);

    for (size_t i = 0; i < (sizeof(FEXCore::Core::CPUState::flags) / sizeof(FEXCore::Core::CPUState::flags[0])); ++i) {
      SetAccess(Offset++, DefaultAccess[6]);
    }

    for (size_t i = 0; i < 8; ++i) {
      SetAccess(Offset++, DefaultAccess[7]);
    }
  }

  struct BlockInfo {
    std::vector<FEXCore::IR::OrderedNode *> Predecessors;
    std::vector<FEXCore::IR::OrderedNode *> Successors;
    ContextInfo IncomingClassifiedStruct;
    ContextInfo OutgoingClassifiedStruct;
  };

class RCLSE final : public FEXCore::IR::Pass {
public:
  RCLSE() {
    ClassifyContextStruct(&ClassifiedStruct);
  }
  bool Run(FEXCore::IR::OpDispatchBuilder *Disp) override;
private:
  ContextInfo ClassifiedStruct;
  std::unordered_map<FEXCore::IR::OrderedNodeWrapper::NodeOffsetType, BlockInfo> OffsetToBlockMap;

  ContextMemberInfo *FindMemberInfo(ContextInfo *ClassifiedInfo, uint32_t Offset, uint8_t Size);
  ContextMemberInfo *RecordAccess(ContextMemberInfo *Info, uint8_t RegClass, uint32_t Offset, uint8_t Size, LastAccessType AccessType, FEXCore::IR::OrderedNode *Node, FEXCore::IR::OrderedNode *Node2 = nullptr);
  ContextMemberInfo *RecordAccess(ContextInfo *ClassifiedInfo, uint8_t RegClass, uint32_t Offset, uint8_t Size, LastAccessType AccessType, FEXCore::IR::OrderedNode *Node, FEXCore::IR::OrderedNode *Node2 = nullptr);
  void CalculateControlFlowInfo(FEXCore::IR::OpDispatchBuilder *Disp);

  // Block local Passes
  bool RedundantStoreLoadElimination(FEXCore::IR::OpDispatchBuilder *Disp);
};

ContextMemberInfo *RCLSE::FindMemberInfo(ContextInfo *ClassifiedInfo, uint32_t Offset, uint8_t Size) {
  ContextMemberInfo *Info{};
  // Just linearly scan to find the info
  for (size_t i = 0; i < ClassifiedInfo->size(); ++i) {
    ContextMemberInfo *LocalInfo = &ClassifiedInfo->at(i);
    if (LocalInfo->Class.Offset <= Offset &&
        (LocalInfo->Class.Offset + LocalInfo->Class.Size) > Offset) {
      Info = LocalInfo;
      break;
    }
  }
  LogMan::Throw::A(Info != nullptr, "Couldn't find Context Member to record to");

  return Info;
}

ContextMemberInfo *RCLSE::RecordAccess(ContextMemberInfo *Info, uint8_t RegClass, uint32_t Offset, uint8_t Size, LastAccessType AccessType, FEXCore::IR::OrderedNode *Node, FEXCore::IR::OrderedNode *Node2) {
  LogMan::Throw::A((Offset + Size) <= (Info->Class.Offset + Info->Class.Size), "Access to context item went over member size");
  LogMan::Throw::A(Info->Accessed != ACCESS_INVALID, "Tried to access invalid member");

  // If we aren't fully overwriting the member then it is a partial write that we need to track
  if (Size < Info->Class.Size) {
    AccessType = AccessType == ACCESS_WRITE ? ACCESS_PARTIAL_WRITE : ACCESS_PARTIAL_READ;
  }
  if (Size > Info->Class.Size) {
    LogMan::Msg::A("Can't handle this");
  }

  Info->Accessed = AccessType;
  Info->AccessRegClass = RegClass;
  Info->AccessOffset = Offset;
  Info->AccessSize = Size;
  Info->Node = Node;
  Info->Node2 = Node2;
  return Info;
}

ContextMemberInfo *RCLSE::RecordAccess(ContextInfo *ClassifiedInfo, uint8_t RegClass, uint32_t Offset, uint8_t Size, LastAccessType AccessType, FEXCore::IR::OrderedNode *Node, FEXCore::IR::OrderedNode *Node2) {
  ContextMemberInfo *Info = FindMemberInfo(ClassifiedInfo, Offset, Size);
  return RecordAccess(Info, RegClass, Offset, Size, AccessType, Node, Node2);
}

void RCLSE::CalculateControlFlowInfo(FEXCore::IR::OpDispatchBuilder *Disp) {
  using namespace FEXCore;
  using namespace FEXCore::IR;

  OffsetToBlockMap.clear();
  auto CurrentIR = Disp->ViewIR();
  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  auto Begin = CurrentIR.begin();
  auto Op = Begin();

  OrderedNode *RealNode = Op->GetNode(ListBegin);
  auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  // Walk the list and calculate the control flow
  OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);
  while (1) {
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    BlockInfo *CurrentBlock = &OffsetToBlockMap.try_emplace(BlockNode->Wrapped(ListBegin).ID()).first->second;

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);

    while (1) {
      auto CodeOp = CodeBegin();
      OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
      auto IROp = CodeNode->Op(DataBegin);

      switch (IROp->Op) {
        case IR::OP_CONDJUMP: {
          auto Op = IROp->CW<IR::IROp_CondJump>();

          OrderedNode *TrueTargetNode = Op->Header.Args[1].GetNode(ListBegin);
          OrderedNode *FalseTargetNode = Op->Header.Args[2].GetNode(ListBegin);

          CurrentBlock->Successors.emplace_back(TrueTargetNode);
          CurrentBlock->Successors.emplace_back(FalseTargetNode);

          {
            auto Block = &OffsetToBlockMap.try_emplace(Op->Header.Args[1].ID()).first->second;
            Block->Predecessors.emplace_back(BlockNode);
          }

          {
            auto Block = &OffsetToBlockMap.try_emplace(Op->Header.Args[2].ID()).first->second;
            Block->Predecessors.emplace_back(BlockNode);
          }

          break;
        }
        case IR::OP_JUMP: {
          auto Op = IROp->CW<IR::IROp_Jump>();
          OrderedNode *TargetNode = Op->Header.Args[0].GetNode(ListBegin);
          CurrentBlock->Successors.emplace_back(TargetNode);

          {
            auto Block = OffsetToBlockMap.try_emplace(Op->Header.Args[0].ID()).first;
            Block->second.Predecessors.emplace_back(BlockNode);
          }
          break;
        }
        default: break;
      }

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (CodeBegin == CodeLast) {
        break;
      }
      ++CodeBegin;
    }

    if (BlockIROp->Next.ID() == 0) {
      break;
    } else {
      BlockNode = BlockIROp->Next.GetNode(ListBegin);
    }
  }
}

/**
 * @brief This pass removes redundant pairs of storecontext and loadcontext ops
 *
 * eg.
 *   %ssa26 i128 = LoadMem %ssa25 i64, 0x10
 *   (%%ssa27) StoreContext %ssa26 i128, 0x10, 0xb0
 *   %ssa28 i128 = LoadContext 0x10, 0x90
 *   %ssa29 i128 = LoadContext 0x10, 0xb0
 * Converts to
 *   %ssa26 i128 = LoadMem %ssa25 i64, 0x10
 *   (%%ssa27) StoreContext %ssa26 i128, 0x10, 0xb0
 *   %ssa28 i128 = LoadContext 0x10, 0x90
 *   %ssa29 i128 = VBitcast %ssa26 i128
 *
 * eg.
 * 		%ssa6 i128 = LoadContext 0x10, 0x90
 *		%ssa7 i128 = LoadContext 0x10, 0x90
 *		%ssa8 i128 = VXor %ssa7 i128, %ssa6 i128
 * Converts to
 *    %ssa6 i128 = LoadContext 0x10, 0x90
 *    %ssa7 i128 = VXor %ssa6 i128, %ssa6 i128
 *
 * eg.
 *   (%%ssa189) StoreContext %ssa188 i128, 0x10, 0xa0
 *   %ssa190 i128 = LoadContext 0x10, 0x90
 *   %ssa191 i128 = VBitcast %ssa188 i128
 *   %ssa192 i128 = VAdd %ssa191 i128, %ssa190 i128, 0x10, 0x4
 *   (%%ssa193) StoreContext %ssa192 i128, 0x10, 0xa0
 * Converts to
 *   %ssa173 i128 = LoadContext 0x10, 0x90
 *   %ssa174 i128 = VBitcast %ssa172 i128
 *   %ssa175 i128 = VAdd %ssa174 i128, %ssa173 i128, 0x10, 0x4
 *   (%%ssa176) StoreContext %ssa175 i128, 0x10, 0xa0

 */
bool RCLSE::RedundantStoreLoadElimination(FEXCore::IR::OpDispatchBuilder *Disp) {
  using namespace FEXCore;
  using namespace FEXCore::IR;

  bool Changed = false;
  auto CurrentIR = Disp->ViewIR();
  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();
  auto OriginalWriteCursor = Disp->GetWriteCursor();

  auto Begin = CurrentIR.begin();
  auto Op = Begin();

  OrderedNode *RealNode = Op->GetNode(ListBegin);
  auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  // Walk the list and calculate the control flow
  OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

  ContextInfo LocalInfo;
  ClassifyContextStruct(&LocalInfo);

  while (1) {
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    ResetClassificationAccesses(&LocalInfo);

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);

    while (1) {
      auto CodeOp = CodeBegin();
      OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
      auto IROp = CodeNode->Op(DataBegin);

      if (IROp->Op == OP_STORECONTEXT) {
        auto Op = IROp->CW<IR::IROp_StoreContext>();
        auto Info = FindMemberInfo(&LocalInfo, Op->Offset, Op->Size);
        uint8_t LastClass = Info->AccessRegClass;
        uint32_t LastOffset = Info->AccessOffset;
        uint8_t LastSize = Info->AccessSize;
        LastAccessType LastAccess = Info->Accessed;
        OrderedNode *LastNode2 = Info->Node2;
        RecordAccess(Info, Op->Class, Op->Offset, Op->Size, ACCESS_WRITE, Op->Header.Args[0].GetNode(ListBegin), CodeNode);

        if (LastAccess == ACCESS_WRITE &&
            LastClass == Op->Class &&
            LastOffset == Op->Offset &&
            LastSize == Op->Size &&
            Info->Accessed == ACCESS_WRITE) {
          // Remove the last store because this one overwrites it entirely
          // Happens when we store in to a location then store again
          Disp->Remove(LastNode2);
        }
      }
      else if (IROp->Op == OP_LOADCONTEXT) {
        auto Op = IROp->CW<IR::IROp_LoadContext>();
        auto Info = FindMemberInfo(&LocalInfo, Op->Offset, Op->Size);
        uint8_t LastClass = Info->AccessRegClass;
        uint32_t LastOffset = Info->AccessOffset;
        uint8_t LastSize = Info->AccessSize;
        LastAccessType LastAccess = Info->Accessed;
        OrderedNode *LastNode = Info->Node;
        RecordAccess(Info, Op->Class, Op->Offset, Op->Size, ACCESS_READ, CodeNode);

        if (LastAccess == ACCESS_WRITE &&
            LastClass == Op->Class &&
            LastOffset == Op->Offset &&
            LastSize == Op->Size &&
            Info->Accessed == ACCESS_READ) {
          // If the last store matches this load value then we can replace the loaded value with the previous valid one
          if (Op->Offset >= offsetof(FEXCore::Core::CPUState, xmm[0]) &&
              Op->Offset <= offsetof(FEXCore::Core::CPUState, xmm[15])) {

            Disp->SetWriteCursor(CodeNode);
            // XMM needs a bit of special help
            auto BitCast = Disp->_VBitcast(Op->Size * 8, 1, LastNode);
            Disp->ReplaceAllUsesWithInclusive(CodeNode, BitCast, CodeBegin, CodeLast);
            RecordAccess(Info, Op->Class, Op->Offset, Op->Size, ACCESS_READ, LastNode);
          }
          else {
            Disp->ReplaceAllUsesWithInclusive(CodeNode, LastNode, CodeBegin, CodeLast);
            RecordAccess(Info, Op->Class, Op->Offset, Op->Size, ACCESS_READ, LastNode);
          }
          Changed = true;
        }
        else if (LastAccess == ACCESS_READ &&
                 LastClass == Op->Class &&
                 LastOffset == Op->Offset &&
                 LastSize == Op->Size &&
                 Info->Accessed == ACCESS_READ) {
          // Did we read and then read again?
          Disp->ReplaceAllUsesWithInclusive(CodeNode, LastNode, CodeBegin, CodeLast);
          RecordAccess(Info, Op->Class, Op->Offset, Op->Size, ACCESS_READ, LastNode);
          Changed = true;
        }
      }
      else if (IROp->Op == OP_STOREFLAG) {
        auto Op = IROp->CW<IR::IROp_StoreFlag>();
        RecordAccess(&LocalInfo, 0, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag, 1, ACCESS_WRITE, Op->Header.Args[0].GetNode(ListBegin), CodeNode);
      }
      else if (IROp->Op == OP_LOADFLAG) {
        auto Op = IROp->CW<IR::IROp_LoadFlag>();
        auto Info = FindMemberInfo(&LocalInfo, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag, 1);
        LastAccessType LastAccess = Info->Accessed;
        OrderedNode *LastNode = Info->Node;
        if (LastAccess == ACCESS_WRITE) { // 1 byte so always a full write
          // If the last store matches this load value then we can replace the loaded value with the previous valid one
          Disp->SetWriteCursor(CodeNode);
          auto Res = Disp->_Bfe(1, 0, LastNode);
          Disp->ReplaceAllUsesWithInclusive(CodeNode, Res, CodeBegin, CodeLast);
          RecordAccess(Info, 0, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag, 1, ACCESS_READ, Res);
          Changed = true;
        }
        else if (LastAccess == ACCESS_READ) {
          Disp->ReplaceAllUsesWithInclusive(CodeNode, LastNode, CodeBegin, CodeLast);
          RecordAccess(Info, 0, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag, 1, ACCESS_READ, LastNode);
          Changed = true;
        }
      }
      else if (IROp->Op == OP_STORECONTEXTINDEXED ||
               IROp->Op == OP_LOADCONTEXTINDEXED) {
        // We can't track through these
        ResetClassificationAccesses(&LocalInfo);
      }

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (CodeBegin == CodeLast) {
        break;
      }
      ++CodeBegin;
    }

    if (BlockIROp->Next.ID() == 0) {
      break;
    } else {
      BlockNode = BlockIROp->Next.GetNode(ListBegin);
    }
  }

  Disp->SetWriteCursor(OriginalWriteCursor);

  return Changed;
}

bool RCLSE::Run(FEXCore::IR::OpDispatchBuilder *Disp) {
  ResetClassificationAccesses(&ClassifiedStruct);
  CalculateControlFlowInfo(Disp);
  bool Changed = false;
  Changed |= RedundantStoreLoadElimination(Disp);

  return Changed;
}

}

namespace FEXCore::IR {

FEXCore::IR::Pass* CreateContextLoadStoreElimination() {
  return new RCLSE{};
}

}
