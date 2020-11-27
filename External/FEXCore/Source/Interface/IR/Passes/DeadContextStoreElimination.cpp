#include "Interface/IR/Passes.h"
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
    FEXCore::IR::RegisterClassType AccessRegClass;
    uint32_t AccessOffset;
    uint8_t AccessSize;
    FEXCore::IR::OrderedNode *Node;
    FEXCore::IR::OrderedNode *StoreNode;
  };

  using ContextInfo = std::vector<ContextMemberInfo>;

  constexpr static std::array<LastAccessType, 14> DefaultAccess = {
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_INVALID, // PAD
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_INVALID, // PAD
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
      FEXCore::IR::InvalidClass,
    });

    for (size_t i = 0; i < 16; ++i) {
      ContextClassification->emplace_back(ContextMemberInfo{
        ContextMemberClassification {
          offsetof(FEXCore::Core::CPUState, gregs[0]) + sizeof(FEXCore::Core::CPUState::gregs[0]) * i,
          sizeof(FEXCore::Core::CPUState::gregs[0]),
        },
        DefaultAccess[1],
        FEXCore::IR::InvalidClass,
      });
    }

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, gregs[16]),
        sizeof(uint64_t),
      },
      DefaultAccess[2], ///< NOP padding
      FEXCore::IR::InvalidClass,
    });

    for (size_t i = 0; i < 16; ++i) {
      ContextClassification->emplace_back(ContextMemberInfo{
        ContextMemberClassification {
          offsetof(FEXCore::Core::CPUState, xmm[0][0]) + sizeof(FEXCore::Core::CPUState::xmm[0]) * i,
          sizeof(FEXCore::Core::CPUState::xmm[0]),
        },
        DefaultAccess[3],
        FEXCore::IR::InvalidClass,
      });
    }

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, gs),
        sizeof(FEXCore::Core::CPUState::gs),
      },
      DefaultAccess[4],
      FEXCore::IR::InvalidClass,
    });

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, es),
        sizeof(FEXCore::Core::CPUState::es),
      },
      DefaultAccess[5],
      FEXCore::IR::InvalidClass,
    });

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, cs),
        sizeof(FEXCore::Core::CPUState::cs),
      },
      DefaultAccess[6],
      FEXCore::IR::InvalidClass,
    });

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, ss),
        sizeof(FEXCore::Core::CPUState::ss),
      },
      DefaultAccess[7],
      FEXCore::IR::InvalidClass,
    });

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, ds),
        sizeof(FEXCore::Core::CPUState::ds),
      },
      DefaultAccess[8],
      FEXCore::IR::InvalidClass,
    });

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, fs),
        sizeof(FEXCore::Core::CPUState::fs),
      },
      DefaultAccess[9],
      FEXCore::IR::InvalidClass,
    });

    for (size_t i = 0; i < (sizeof(FEXCore::Core::CPUState::flags) / sizeof(FEXCore::Core::CPUState::flags[0])); ++i) {
      ContextClassification->emplace_back(ContextMemberInfo{
        ContextMemberClassification {
          offsetof(FEXCore::Core::CPUState, flags[0]) + sizeof(FEXCore::Core::CPUState::flags[0]) * i,
          sizeof(FEXCore::Core::CPUState::flags[0]),
        },
        DefaultAccess[10],
        FEXCore::IR::InvalidClass,
      });
    }

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, flags[48]),
        sizeof(uint64_t),
      },
      DefaultAccess[11], ///< NOP padding
      FEXCore::IR::InvalidClass,
    });

    for (size_t i = 0; i < 8; ++i) {
      ContextClassification->emplace_back(ContextMemberInfo{
        ContextMemberClassification {
          offsetof(FEXCore::Core::CPUState, mm[0][0]) + sizeof(FEXCore::Core::CPUState::mm[0]) * i,
          sizeof(FEXCore::Core::CPUState::mm[0]),
        },
        DefaultAccess[12],
        FEXCore::IR::InvalidClass,
      });
    }

    // GDTs
    for (size_t i = 0; i < 32; ++i) {
      ContextClassification->emplace_back(ContextMemberInfo{
        ContextMemberClassification {
          offsetof(FEXCore::Core::CPUState, gdt[0]) + sizeof(FEXCore::Core::CPUState::gdt[0]) * i,
          sizeof(FEXCore::Core::CPUState::gdt[0]),
        },
        DefaultAccess[13],
        FEXCore::IR::InvalidClass,
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
      ContextClassification->at(Offset).AccessRegClass = FEXCore::IR::InvalidClass;
      ContextClassification->at(Offset).AccessOffset = 0;
      ContextClassification->at(Offset).StoreNode = nullptr;
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
    SetAccess(Offset++, DefaultAccess[6]);
    SetAccess(Offset++, DefaultAccess[7]);
    SetAccess(Offset++, DefaultAccess[8]);
    SetAccess(Offset++, DefaultAccess[9]);


    for (size_t i = 0; i < (sizeof(FEXCore::Core::CPUState::flags) / sizeof(FEXCore::Core::CPUState::flags[0])); ++i) {
      SetAccess(Offset++, DefaultAccess[10]);
    }

    SetAccess(Offset++, DefaultAccess[11]);

    for (size_t i = 0; i < 8; ++i) {
      SetAccess(Offset++, DefaultAccess[12]);
    }

    for (size_t i = 0; i < 32; ++i) {
      SetAccess(Offset++, DefaultAccess[13]);
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
    DCE.reset(FEXCore::IR::CreatePassDeadCodeElimination());
  }
  bool Run(FEXCore::IR::IREmitter *IREmit) override;
private:
  std::unique_ptr<FEXCore::IR::Pass> DCE;

  ContextInfo ClassifiedStruct;
  std::unordered_map<FEXCore::IR::OrderedNodeWrapper::NodeOffsetType, BlockInfo> OffsetToBlockMap;

  ContextMemberInfo *FindMemberInfo(ContextInfo *ClassifiedInfo, uint32_t Offset, uint8_t Size);
  ContextMemberInfo *RecordAccess(ContextMemberInfo *Info, FEXCore::IR::RegisterClassType RegClass, uint32_t Offset, uint8_t Size, LastAccessType AccessType, FEXCore::IR::OrderedNode *Node, FEXCore::IR::OrderedNode *StoreNode = nullptr);
  ContextMemberInfo *RecordAccess(ContextInfo *ClassifiedInfo, FEXCore::IR::RegisterClassType RegClass, uint32_t Offset, uint8_t Size, LastAccessType AccessType, FEXCore::IR::OrderedNode *Node, FEXCore::IR::OrderedNode *StoreNode = nullptr);
  void CalculateControlFlowInfo(FEXCore::IR::IREmitter *IREmit);

  // Block local Passes
  bool RedundantStoreLoadElimination(FEXCore::IR::IREmitter *IREmit);
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

ContextMemberInfo *RCLSE::RecordAccess(ContextMemberInfo *Info, FEXCore::IR::RegisterClassType RegClass, uint32_t Offset, uint8_t Size, LastAccessType AccessType, FEXCore::IR::OrderedNode *Node, FEXCore::IR::OrderedNode *StoreNode) {
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
  if (StoreNode != nullptr)
    Info->StoreNode = StoreNode;
  return Info;
}

ContextMemberInfo *RCLSE::RecordAccess(ContextInfo *ClassifiedInfo, FEXCore::IR::RegisterClassType RegClass, uint32_t Offset, uint8_t Size, LastAccessType AccessType, FEXCore::IR::OrderedNode *Node, FEXCore::IR::OrderedNode *StoreNode) {
  ContextMemberInfo *Info = FindMemberInfo(ClassifiedInfo, Offset, Size);
  return RecordAccess(Info, RegClass, Offset, Size, AccessType, Node, StoreNode);
}

void RCLSE::CalculateControlFlowInfo(FEXCore::IR::IREmitter *IREmit) {
  using namespace FEXCore;
  using namespace FEXCore::IR;

  OffsetToBlockMap.clear();
  auto CurrentIR = IREmit->ViewIR();

  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
    BlockInfo *CurrentBlock = &OffsetToBlockMap.try_emplace(CurrentIR.GetID(BlockNode)).first->second;

    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {

      switch (IROp->Op) {
        case IR::OP_CONDJUMP: {
          auto Op = IROp->CW<IR::IROp_CondJump>();

          OrderedNode *TrueTargetNode = CurrentIR.GetNode(Op->TrueBlock);
          OrderedNode *FalseTargetNode = CurrentIR.GetNode(Op->FalseBlock);

          CurrentBlock->Successors.emplace_back(TrueTargetNode);
          CurrentBlock->Successors.emplace_back(FalseTargetNode);

          {
            auto Block = &OffsetToBlockMap.try_emplace(Op->TrueBlock.ID()).first->second;
            Block->Predecessors.emplace_back(BlockNode);
          }

          {
            auto Block = &OffsetToBlockMap.try_emplace(Op->FalseBlock.ID()).first->second;
            Block->Predecessors.emplace_back(BlockNode);
          }

          break;
        }
        case IR::OP_JUMP: {
          auto Op = IROp->CW<IR::IROp_Jump>();
          OrderedNode *TargetNode = CurrentIR.GetNode(Op->Header.Args[0]);
          CurrentBlock->Successors.emplace_back(TargetNode);

          {
            auto Block = OffsetToBlockMap.try_emplace(Op->Header.Args[0].ID()).first;
            Block->second.Predecessors.emplace_back(BlockNode);
          }
          break;
        }
        default: break;
      }
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
bool RCLSE::RedundantStoreLoadElimination(FEXCore::IR::IREmitter *IREmit) {
  using namespace FEXCore;
  using namespace FEXCore::IR;

  bool Changed = false;
  auto CurrentIR = IREmit->ViewIR();
  auto OriginalWriteCursor = IREmit->GetWriteCursor();

  // XXX: Walk the list and calculate the control flow

  ContextInfo LocalInfo = ClassifiedStruct;

  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {

    ResetClassificationAccesses(&LocalInfo);

    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      if (IROp->Op == OP_STORECONTEXT) {
        auto Op = IROp->CW<IR::IROp_StoreContext>();
        auto Info = FindMemberInfo(&LocalInfo, Op->Offset, IROp->Size);
        uint8_t LastClass = Info->AccessRegClass;
        uint32_t LastOffset = Info->AccessOffset;
        uint8_t LastSize = Info->AccessSize;
        LastAccessType LastAccess = Info->Accessed;
        OrderedNode *LastStoreNode = Info->StoreNode;
        RecordAccess(Info, Op->Class, Op->Offset, IROp->Size, ACCESS_WRITE, CurrentIR.GetNode(Op->Header.Args[0]), CodeNode);

        if ((LastAccess == ACCESS_WRITE || LastAccess == ACCESS_PARTIAL_WRITE) &&
            LastClass == Op->Class &&
            LastOffset == Op->Offset &&
            LastSize <= IROp->Size) {
          // Remove the last store because this one overwrites it entirely
          // Happens when we store in to a location then store again
          IREmit->Remove(LastStoreNode);

          if (LastSize < IROp->Size) {
            //printf("RCLSE: Eliminated partial write\n");
          }
          Changed = true;
        }
      }
      else if (IROp->Op == OP_LOADCONTEXT) {
        auto Op = IROp->CW<IR::IROp_LoadContext>();
        auto Info = FindMemberInfo(&LocalInfo, Op->Offset, IROp->Size);
        RegisterClassType LastClass = Info->AccessRegClass;
        uint32_t LastOffset = Info->AccessOffset;
        uint8_t LastSize = Info->AccessSize;
        LastAccessType LastAccess = Info->Accessed;
        OrderedNode *LastNode = Info->Node;
        OrderedNode *LastStoreNode = Info->StoreNode;
        RecordAccess(Info, Op->Class, Op->Offset, IROp->Size, ACCESS_READ, CodeNode);

        if ((LastAccess == ACCESS_WRITE || LastAccess == ACCESS_PARTIAL_WRITE) &&
            LastClass == Op->Class &&
            LastOffset == Op->Offset &&
            IROp->Size <= LastSize) {
          // If the last store matches this load value then we can replace the loaded value with the previous valid one

          if (LastClass == GPRClass) {
            IREmit->SetWriteCursor(CodeNode);

            uint8_t TruncateSize = IREmit->GetOpSize(LastNode);

            // Did store context do an implicit truncation?
            if (IREmit->GetOpSize(LastStoreNode) < TruncateSize)
              TruncateSize = IREmit->GetOpSize(LastStoreNode);

            // Or are we doing a partial read
            if (IROp->Size < TruncateSize)
              TruncateSize = IROp->Size;

            if (TruncateSize != IREmit->GetOpSize(LastNode)) {
              // We need to insert an explict truncation
              if (LastClass == FPRClass) {
                LastNode = IREmit->_VMov(LastNode, TruncateSize); // Vmov truncates and zexts when register width is smaller than source
              }
              else if (LastClass == GPRPairClass) {
                LastNode = IREmit->_TruncElementPair(LastNode, TruncateSize);
              }
              else if (LastClass == GPRClass) {
                LastNode = IREmit->_Bfe(Info->AccessSize, TruncateSize * 8, 0, LastNode);
              } else {
                LogMan::Msg::A("Unhandled Register class");
              }
            }

            IREmit->ReplaceAllUsesWith(CodeNode, LastNode);
            RecordAccess(Info, Op->Class, Op->Offset, IROp->Size, ACCESS_READ, LastNode);
            Changed = true;
          } else {
            if (LastClass == FPRClass && LastSize == IROp->Size && LastSize == IREmit->GetOpSize(LastNode)) {
              // LoadCtx matches StoreCtx and Node Size
              IREmit->ReplaceAllUsesWith(CodeNode, LastNode);
              RecordAccess(Info, Op->Class, Op->Offset, IROp->Size, ACCESS_READ, LastNode);
              Changed = true;
            } else if (LastClass == FPRClass && LastSize >= IROp->Size && IROp->Size == IREmit->GetOpSize(LastNode)) {
              // LoadCtx is <= StoreCtx and Node is LoadCtx
              IREmit->ReplaceAllUsesWith(CodeNode, LastNode);
              RecordAccess(Info, Op->Class, Op->Offset, IROp->Size, ACCESS_READ, LastNode);
              Changed = true;
            } else if (LastClass == FPRClass && LastSize >= IROp->Size && IROp->Size < IREmit->GetOpSize(LastNode)) {
              IREmit->SetWriteCursor(CodeNode);
              // trucate to size
              LastNode = IREmit->_VMov(LastNode, IROp->Size);
              IREmit->ReplaceAllUsesWith(CodeNode, LastNode);
              RecordAccess(Info, Op->Class, Op->Offset, IROp->Size, ACCESS_READ, LastNode);
              Changed = true;
            } else if (LastClass == FPRClass && LastSize >= IROp->Size && IROp->Size > IREmit->GetOpSize(LastNode)) {
              IREmit->SetWriteCursor(CodeNode);
              // zext to size
              LastNode = IREmit->_VMov(LastNode, IROp->Size);

              IREmit->ReplaceAllUsesWith(CodeNode, LastNode);
              RecordAccess(Info, Op->Class, Op->Offset, IROp->Size, ACCESS_READ, LastNode);
              Changed = true;
            } else {
              //printf("RCLSE: Not GPR class, missed, %d, lastS: %d, S: %d, Node S: %d\n", LastClass, LastSize, IROp->Size, IREmit->GetOpSize(LastNode));
            }
          }
        }
        else if ((LastAccess == ACCESS_READ || LastAccess == ACCESS_PARTIAL_READ) &&
                 LastClass == Op->Class &&
                 LastOffset == Op->Offset &&
                 LastSize == IROp->Size &&
                 (Info->Accessed == ACCESS_READ || Info->Accessed == ACCESS_PARTIAL_READ)) {
          // Did we read and then read again?
          IREmit->ReplaceAllUsesWith(CodeNode, LastNode);
          RecordAccess(Info, Op->Class, Op->Offset, IROp->Size, ACCESS_READ, LastNode);
          Changed = true;
        }
      }
      else if (IROp->Op == OP_STOREFLAG) {
        auto Op = IROp->CW<IR::IROp_StoreFlag>();
        auto Info = FindMemberInfo(&LocalInfo, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag, 1);
        auto LastStoreNode = Info->StoreNode;
        RecordAccess(&LocalInfo, FEXCore::IR::GPRClass, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag, 1, ACCESS_WRITE, CurrentIR.GetNode(Op->Header.Args[0]), CodeNode);

        // Flags don't alias, so we can take the simple route here. Kill any flags that have been overwritten
        if (LastStoreNode != nullptr)
        {
          IREmit->Remove(LastStoreNode);
          Changed = true;
        }
      }
      else if (IROp->Op == OP_LOADFLAG) {
        auto Op = IROp->CW<IR::IROp_LoadFlag>();
        auto Info = FindMemberInfo(&LocalInfo, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag, 1);
        LastAccessType LastAccess = Info->Accessed;
        OrderedNode *LastNode = Info->Node;

        if (LastAccess == ACCESS_WRITE) { // 1 byte so always a full write
          // If the last store matches this load value then we can replace the loaded value with the previous valid one
          IREmit->SetWriteCursor(CodeNode);
          IREmit->ReplaceAllUsesWith(CodeNode, LastNode);
          RecordAccess(Info, FEXCore::IR::GPRClass, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag, 1, ACCESS_READ, LastNode);
          Changed = true;
        }
        else if (LastAccess == ACCESS_READ) {
          IREmit->ReplaceAllUsesWith(CodeNode, LastNode);
          RecordAccess(Info, FEXCore::IR::GPRClass, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag, 1, ACCESS_READ, LastNode);
          Changed = true;
        }
      }
      else if (IROp->Op == OP_STORECONTEXTINDEXED ||
               IROp->Op == OP_LOADCONTEXTINDEXED ||
               IROp->Op == OP_LOADCONTEXTPAIR ||
               IROp->Op == OP_STORECONTEXTPAIR) {
        // We can't track through these
        ResetClassificationAccesses(&LocalInfo);
      }
    }
  }

  IREmit->SetWriteCursor(OriginalWriteCursor);

  return Changed;
}

bool RCLSE::Run(FEXCore::IR::IREmitter *IREmit) {
  // XXX: We don't do cross-block optimizations yet
  //CalculateControlFlowInfo(IREmit);
  bool Changed = false;
  
  // Run up to 5 times
  for( int i = 0; i < 5 && RedundantStoreLoadElimination(IREmit); i++) {
    Changed = true;
    DCE->Run(IREmit);
  }

  return Changed;
}

}

namespace FEXCore::IR {

FEXCore::IR::Pass* CreateContextLoadStoreElimination() {
  return new RCLSE{};
}

}
