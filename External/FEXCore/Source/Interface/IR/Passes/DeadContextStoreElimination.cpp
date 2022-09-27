/*
$info$
tags: ir|opts
desc: Transforms ContextLoad/Store to temporaries, similar to mem2reg
$end_info$
*/

#include "Interface/IR/Passes.h"
#include "Interface/IR/PassManager.h"
#include <FEXCore/Core/CoreState.h>

#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IREmitter.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/Utils/LogManager.h>

#include <array>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
  struct ContextMemberClassification {
    size_t Offset;
    uint16_t Size;
  };

  enum LastAccessType {
    ACCESS_NONE      = (0b000 << 0), ///< Was never previously accessed
    ACCESS_WRITE     = (0b001 << 0), ///< Was fully overwritten
    ACCESS_READ      = (0b010 << 0), ///< Was fully read
    ACCESS_INVALID   = (0b011 << 0), ///< Accessing this is invalid
    ACCESS_TYPE_MASK = (0b011 << 0),
    ACCESS_PARTIAL   = (0b100 << 0),
    ACCESS_PARTIAL_WRITE = (ACCESS_PARTIAL | ACCESS_WRITE), ///< Was partially written
    ACCESS_PARTIAL_READ  = (ACCESS_PARTIAL | ACCESS_READ),  ///< Was partially read
  };

  static bool IsWriteAccess(LastAccessType Type) {
    return (Type & ACCESS_TYPE_MASK) == ACCESS_WRITE;
  }

  static bool IsReadAccess(LastAccessType Type) {
    return (Type & ACCESS_TYPE_MASK) == ACCESS_READ;
  }

  [[maybe_unused]]
  static bool IsInvalidAccess(LastAccessType Type) {
    return (Type & ACCESS_TYPE_MASK) == ACCESS_INVALID;
  }

  [[maybe_unused]]
  static bool IsPartialAccess(LastAccessType Type) {
    return (Type & ACCESS_PARTIAL) == ACCESS_PARTIAL;
  }

  static bool IsFullAccess(LastAccessType Type) {
    return (Type & ACCESS_PARTIAL) == 0;
  }

  struct ContextMemberInfo {
    ContextMemberClassification Class;
    LastAccessType Accessed;
    FEXCore::IR::RegisterClassType AccessRegClass;
    uint32_t AccessOffset;
    uint8_t AccessSize;
    FEXCore::IR::OrderedNode *Node;
    FEXCore::IR::OrderedNode *StoreNode;
  };

  struct ContextInfo {
    std::vector<ContextMemberInfo*> Lookup;
    std::vector<ContextMemberInfo> ClassificationInfo;
  };

  constexpr static std::array<LastAccessType, 15> DefaultAccess = {
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_INVALID, // SSE padding in non-AVX case
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
    ACCESS_NONE,
  };

  static void ClassifyContextStruct(ContextInfo *ContextClassificationInfo, bool SupportsAVX) {
    auto ContextClassification = &ContextClassificationInfo->ClassificationInfo;

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, rip),
        sizeof(FEXCore::Core::CPUState::rip),
      },
      DefaultAccess[0],
      FEXCore::IR::InvalidClass,
    });

    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_GPRS; ++i) {
      ContextClassification->emplace_back(ContextMemberInfo{
        ContextMemberClassification {
          offsetof(FEXCore::Core::CPUState, gregs[0]) + sizeof(FEXCore::Core::CPUState::gregs[0]) * i,
          FEXCore::Core::CPUState::GPR_REG_SIZE,
        },
        DefaultAccess[1],
        FEXCore::IR::InvalidClass,
      });
    }

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, es),
        sizeof(FEXCore::Core::CPUState::es),
      },
      DefaultAccess[2],
      FEXCore::IR::InvalidClass,
    });

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, cs),
        sizeof(FEXCore::Core::CPUState::cs),
      },
      DefaultAccess[3],
      FEXCore::IR::InvalidClass,
    });

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, ss),
        sizeof(FEXCore::Core::CPUState::ss),
      },
      DefaultAccess[4],
      FEXCore::IR::InvalidClass,
    });

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, ds),
        sizeof(FEXCore::Core::CPUState::ds),
      },
      DefaultAccess[5],
      FEXCore::IR::InvalidClass,
    });

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, gs),
        sizeof(FEXCore::Core::CPUState::gs),
      },
      DefaultAccess[6],
      FEXCore::IR::InvalidClass,
    });

    ContextClassification->emplace_back(ContextMemberInfo{
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, fs),
        sizeof(FEXCore::Core::CPUState::fs),
      },
      DefaultAccess[7],
      FEXCore::IR::InvalidClass,
    });

    if (SupportsAVX) {
      for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_XMMS; ++i) {
        ContextClassification->emplace_back(ContextMemberInfo{
          ContextMemberClassification {
            offsetof(FEXCore::Core::CPUState, xmm.avx.data[0][0]) + FEXCore::Core::CPUState::XMM_AVX_REG_SIZE * i,
            FEXCore::Core::CPUState::XMM_AVX_REG_SIZE,
          },
          DefaultAccess[8],
          FEXCore::IR::InvalidClass,
        });
      }
    } else {
      for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_XMMS; ++i) {
        ContextClassification->emplace_back(ContextMemberInfo{
          ContextMemberClassification {
            offsetof(FEXCore::Core::CPUState, xmm.sse.data[0][0]) + FEXCore::Core::CPUState::XMM_SSE_REG_SIZE * i,
            FEXCore::Core::CPUState::XMM_SSE_REG_SIZE,
          },
          DefaultAccess[8],
          FEXCore::IR::InvalidClass,
        });
      }

      ContextClassification->emplace_back(ContextMemberInfo{
          ContextMemberClassification {
            offsetof(FEXCore::Core::CPUState, xmm.sse.pad[0][0]),
            static_cast<uint16_t>(FEXCore::Core::CPUState::XMM_SSE_REG_SIZE * FEXCore::Core::CPUState::NUM_XMMS),
          },
          DefaultAccess[9],
          FEXCore::IR::InvalidClass,
        });
    }

    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_FLAGS; ++i) {
      ContextClassification->emplace_back(ContextMemberInfo{
        ContextMemberClassification {
          offsetof(FEXCore::Core::CPUState, flags[0]) + sizeof(FEXCore::Core::CPUState::flags[0]) * i,
          FEXCore::Core::CPUState::FLAG_SIZE,
        },
        DefaultAccess[10],
        FEXCore::IR::InvalidClass,
      });
    }

    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_MMS; ++i) {
      ContextClassification->emplace_back(ContextMemberInfo{
        ContextMemberClassification {
          offsetof(FEXCore::Core::CPUState, mm[0][0]) + sizeof(FEXCore::Core::CPUState::mm[0]) * i,
          FEXCore::Core::CPUState::MM_REG_SIZE
        },
        DefaultAccess[11],
        FEXCore::IR::InvalidClass,
      });
    }

    // GDTs
    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_GDTS; ++i) {
      ContextClassification->emplace_back(ContextMemberInfo{
        ContextMemberClassification {
          offsetof(FEXCore::Core::CPUState, gdt[0]) + sizeof(FEXCore::Core::CPUState::gdt[0]) * i,
          sizeof(FEXCore::Core::CPUState::gdt[0]),
        },
        DefaultAccess[12],
        FEXCore::IR::InvalidClass,
      });
    }

    // FCW
    ContextClassification->emplace_back(ContextMemberInfo {
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, FCW),
        sizeof(FEXCore::Core::CPUState::FCW),
      },
      DefaultAccess[13],
      FEXCore::IR::InvalidClass,
    });

    // FTW
    ContextClassification->emplace_back(ContextMemberInfo {
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, FTW),
        sizeof(FEXCore::Core::CPUState::FTW),
      },
      DefaultAccess[14],
      FEXCore::IR::InvalidClass,
    });


    [[maybe_unused]] size_t ClassifiedStructSize{};
    ContextClassificationInfo->Lookup.reserve(sizeof(FEXCore::Core::CPUState));
    for (auto &it : *ContextClassification) {
      LOGMAN_THROW_A_FMT(it.Class.Offset == ContextClassificationInfo->Lookup.size(), "Offset mismatch (offset={})", it.Class.Offset);
      for (int i = 0; i < it.Class.Size; i++) {
        ContextClassificationInfo->Lookup.push_back(&it);
      }
      ClassifiedStructSize += it.Class.Size;
    }

    LOGMAN_THROW_AA_FMT(ClassifiedStructSize == sizeof(FEXCore::Core::CPUState),
      "Classified CPUStruct size doesn't match real CPUState struct size! {} (classified) != {} (real)",
      ClassifiedStructSize, sizeof(FEXCore::Core::CPUState));

    LOGMAN_THROW_A_FMT(ContextClassificationInfo->Lookup.size() == sizeof(FEXCore::Core::CPUState),
      "Classified lookup size doesn't match real CPUState struct size! {} (classified) != {} (real)",
      ContextClassificationInfo->Lookup.size(), sizeof(FEXCore::Core::CPUState));
  }

  static void ResetClassificationAccesses(ContextInfo *ContextClassificationInfo, bool SupportsAVX) {
    auto ContextClassification = &ContextClassificationInfo->ClassificationInfo;

    auto SetAccess = [&](size_t Offset, auto Access) {
      ContextClassification->at(Offset).Accessed = Access;
      ContextClassification->at(Offset).AccessRegClass = FEXCore::IR::InvalidClass;
      ContextClassification->at(Offset).AccessOffset = 0;
      ContextClassification->at(Offset).StoreNode = nullptr;
    };
    size_t Offset = 0;
    SetAccess(Offset++, DefaultAccess[0]);
    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_GPRS; ++i) {
      SetAccess(Offset++, DefaultAccess[1]);
    }

    SetAccess(Offset++, DefaultAccess[2]);
    SetAccess(Offset++, DefaultAccess[3]);
    SetAccess(Offset++, DefaultAccess[4]);
    SetAccess(Offset++, DefaultAccess[5]);
    SetAccess(Offset++, DefaultAccess[6]);
    SetAccess(Offset++, DefaultAccess[7]);

    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_XMMS; ++i) {
      SetAccess(Offset++, DefaultAccess[8]);
    }

    if (!SupportsAVX) {
      SetAccess(Offset++, DefaultAccess[9]);
    }

    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_FLAGS; ++i) {
      SetAccess(Offset++, DefaultAccess[10]);
    }

    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_MMS; ++i) {
      SetAccess(Offset++, DefaultAccess[11]);
    }

    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_GDTS; ++i) {
      SetAccess(Offset++, DefaultAccess[12]);
    }

    SetAccess(Offset++, DefaultAccess[13]);
    SetAccess(Offset++, DefaultAccess[14]);
  }

  struct BlockInfo {
    std::vector<FEXCore::IR::OrderedNode *> Predecessors;
    std::vector<FEXCore::IR::OrderedNode *> Successors;
    ContextInfo IncomingClassifiedStruct;
    ContextInfo OutgoingClassifiedStruct;
  };

class RCLSE final : public FEXCore::IR::Pass {
public:
  explicit RCLSE(bool SupportsAVX_) : SupportsAVX{SupportsAVX_} {
    ClassifyContextStruct(&ClassifiedStruct, SupportsAVX);
    DCE = FEXCore::IR::CreatePassDeadCodeElimination();
  }
  bool Run(FEXCore::IR::IREmitter *IREmit) override;
private:
  std::unique_ptr<FEXCore::IR::Pass> DCE;

  ContextInfo ClassifiedStruct;
  std::unordered_map<FEXCore::IR::NodeID, BlockInfo> OffsetToBlockMap;

  bool SupportsAVX;

  ContextMemberInfo *FindMemberInfo(ContextInfo *ClassifiedInfo, uint32_t Offset, uint8_t Size);
  ContextMemberInfo *RecordAccess(ContextMemberInfo *Info, FEXCore::IR::RegisterClassType RegClass, uint32_t Offset, uint8_t Size, LastAccessType AccessType, FEXCore::IR::OrderedNode *Node, FEXCore::IR::OrderedNode *StoreNode = nullptr);
  ContextMemberInfo *RecordAccess(ContextInfo *ClassifiedInfo, FEXCore::IR::RegisterClassType RegClass, uint32_t Offset, uint8_t Size, LastAccessType AccessType, FEXCore::IR::OrderedNode *Node, FEXCore::IR::OrderedNode *StoreNode = nullptr);
  void CalculateControlFlowInfo(FEXCore::IR::IREmitter *IREmit);

  // Block local Passes
  bool RedundantStoreLoadElimination(FEXCore::IR::IREmitter *IREmit);
};

ContextMemberInfo *RCLSE::FindMemberInfo(ContextInfo *ContextClassificationInfo, uint32_t Offset, uint8_t Size) {
  return ContextClassificationInfo->Lookup.at(Offset);
}

ContextMemberInfo *RCLSE::RecordAccess(ContextMemberInfo *Info, FEXCore::IR::RegisterClassType RegClass, uint32_t Offset, uint8_t Size, LastAccessType AccessType, FEXCore::IR::OrderedNode *Node, FEXCore::IR::OrderedNode *StoreNode) {
  LOGMAN_THROW_AA_FMT((Offset + Size) <= (Info->Class.Offset + Info->Class.Size), "Access to context item went over member size");
  LOGMAN_THROW_AA_FMT(Info->Accessed != ACCESS_INVALID, "Tried to access invalid member");

  // If we aren't fully overwriting the member then it is a partial write that we need to track
  if (Size < Info->Class.Size) {
    AccessType = AccessType == ACCESS_WRITE ? ACCESS_PARTIAL_WRITE : ACCESS_PARTIAL_READ;
  }
  if (Size > Info->Class.Size) {
    LOGMAN_MSG_A_FMT("Can't handle this");
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
 *   %ssa192 i128 = VAdd %ssa188 i128, %ssa190 i128, 0x10, 0x4
 *   (%%ssa193) StoreContext %ssa192 i128, 0x10, 0xa0
 * Converts to
 *   %ssa173 i128 = LoadContext 0x10, 0x90
 *   %ssa175 i128 = VAdd %ssa172 i128, %ssa173 i128, 0x10, 0x4
 *   (%%ssa176) StoreContext %ssa175 i128, 0x10, 0xa0

 */
bool RCLSE::RedundantStoreLoadElimination(FEXCore::IR::IREmitter *IREmit) {
  using namespace FEXCore;
  using namespace FEXCore::IR;

  bool Changed = false;
  auto CurrentIR = IREmit->ViewIR();
  auto OriginalWriteCursor = IREmit->GetWriteCursor();

  // XXX: Walk the list and calculate the control flow

  ContextInfo &LocalInfo = ClassifiedStruct;

  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
    auto BlockOp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();
    auto BlockEnd = IREmit->GetIterator(BlockOp->Last);

    ResetClassificationAccesses(&LocalInfo, SupportsAVX);

    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      if (IROp->Op == OP_STORECONTEXT) {
        auto Op = IROp->CW<IR::IROp_StoreContext>();
        auto Info = FindMemberInfo(&LocalInfo, Op->Offset, IROp->Size);
        uint8_t LastClass = Info->AccessRegClass;
        uint32_t LastOffset = Info->AccessOffset;
        uint8_t LastSize = Info->AccessSize;
        LastAccessType LastAccess = Info->Accessed;
        OrderedNode *LastStoreNode = Info->StoreNode;
        RecordAccess(Info, Op->Class, Op->Offset, IROp->Size, ACCESS_WRITE, CurrentIR.GetNode(Op->Value), CodeNode);

        if (IsWriteAccess(LastAccess) &&
            LastClass == Op->Class &&
            LastOffset == Op->Offset &&
            LastSize <= IROp->Size) {
          // Remove the last store because this one overwrites it entirely
          // Happens when we store in to a location then store again
          IREmit->Remove(LastStoreNode);

          if (LastSize < IROp->Size) {
            //fmt::print("RCLSE: Eliminated partial write\n");
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

        if (IsWriteAccess(LastAccess) &&
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
              LastNode = IREmit->_Bfe(Info->AccessSize, TruncateSize * 8, 0, LastNode);
            }

            IREmit->ReplaceAllUsesWithRange(CodeNode, LastNode, IREmit->GetIterator(IREmit->WrapNode(CodeNode)), BlockEnd);
            RecordAccess(Info, Op->Class, Op->Offset, IROp->Size, ACCESS_READ, LastNode);
            Changed = true;
          } else if (LastClass == FPRClass) {
            if (LastSize == IROp->Size && LastSize == IREmit->GetOpSize(LastNode)) {
              if (IsFullAccess(Info->Accessed)) {
                // LoadCtx matches StoreCtx and Node Size
                IREmit->ReplaceAllUsesWithRange(CodeNode, LastNode, IREmit->GetIterator(IREmit->WrapNode(CodeNode)), BlockEnd);
                RecordAccess(Info, Op->Class, Op->Offset, IROp->Size, ACCESS_READ, LastNode);
                Changed = true;
              }
              else {
                // If this load size is a partial load then it may be expecting a zext of
                // the vector element
                IREmit->SetWriteCursor(CodeNode);
                // zext to size
                LastNode = IREmit->_VMov(IROp->Size, LastNode);

                IREmit->ReplaceAllUsesWithRange(CodeNode, LastNode, IREmit->GetIterator(IREmit->WrapNode(CodeNode)), BlockEnd);
                RecordAccess(Info, Op->Class, Op->Offset, IROp->Size, ACCESS_READ, LastNode);
                Changed = true;
              }
            } else if (LastSize >= IROp->Size &&
                       IROp->Size == IREmit->GetOpSize(LastNode)) {
              // LoadCtx is <= StoreCtx and Node is LoadCtx
              IREmit->ReplaceAllUsesWithRange(CodeNode, LastNode, IREmit->GetIterator(IREmit->WrapNode(CodeNode)), BlockEnd);
              RecordAccess(Info, Op->Class, Op->Offset, IROp->Size, ACCESS_READ, LastNode);
              Changed = true;
            } else if (LastSize >= IROp->Size &&
                       IROp->Size < IREmit->GetOpSize(LastNode)) {
              IREmit->SetWriteCursor(CodeNode);
              // trucate to size
              LastNode = IREmit->_VMov(IROp->Size, LastNode);
              IREmit->ReplaceAllUsesWithRange(CodeNode, LastNode, IREmit->GetIterator(IREmit->WrapNode(CodeNode)), BlockEnd);
              RecordAccess(Info, Op->Class, Op->Offset, IROp->Size, ACCESS_READ, LastNode);
              Changed = true;
            } else if (LastSize >= IROp->Size &&
                       IROp->Size > IREmit->GetOpSize(LastNode)) {
              IREmit->SetWriteCursor(CodeNode);
              // zext to size
              LastNode = IREmit->_VMov(IROp->Size, LastNode);

              IREmit->ReplaceAllUsesWithRange(CodeNode, LastNode, IREmit->GetIterator(IREmit->WrapNode(CodeNode)), BlockEnd);
              RecordAccess(Info, Op->Class, Op->Offset, IROp->Size, ACCESS_READ, LastNode);
              Changed = true;
            } else {
              //fmt::print("RCLSE: Not GPR class, missed, {}, lastS: {}, S: {}, Node S: {}\n", LastClass, LastSize, IROp->Size, IREmit->GetOpSize(LastNode));
            }
          }
        }
        else if (IsReadAccess(LastAccess) &&
                 IsReadAccess(Info->Accessed) &&
                 LastClass == Op->Class &&
                 LastOffset == Op->Offset &&
                 LastSize == IROp->Size) {
          // Did we read and then read again?
          IREmit->ReplaceAllUsesWithRange(CodeNode, LastNode, IREmit->GetIterator(IREmit->WrapNode(CodeNode)), BlockEnd);
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
      else if (IROp->Op == OP_INVALIDATEFLAGS) {
        auto Op = IROp->CW<IR::IROp_InvalidateFlags>();

        // Loop through non-reserved flag stores and eliminate unused ones.
        for (size_t F = 0; F < Core::CPUState::NUM_EFLAG_BITS; F++) {
          if (!(Op->Flags & (1ULL << F))) {
            continue;
          }

          auto Info = FindMemberInfo(&LocalInfo, offsetof(FEXCore::Core::CPUState, flags[0]) + F, 1);
          auto LastStoreNode = Info->StoreNode;

          // Flags don't alias, so we can take the simple route here. Kill any flags that have been invalidated without a read.
          if (LastStoreNode != nullptr)
          {
            IREmit->SetWriteCursor(CodeNode);
            RecordAccess(&LocalInfo, FEXCore::IR::GPRClass, offsetof(FEXCore::Core::CPUState, flags[0]) + F, 1, ACCESS_WRITE, IREmit->_Constant(0), CodeNode);

            IREmit->Remove(LastStoreNode);
            Changed = true;
          }
        }
      }
      else if (IROp->Op == OP_LOADFLAG) {
        auto Op = IROp->CW<IR::IROp_LoadFlag>();
        auto Info = FindMemberInfo(&LocalInfo, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag, 1);
        LastAccessType LastAccess = Info->Accessed;
        OrderedNode *LastNode = Info->Node;

        if (IsWriteAccess(LastAccess)) { // 1 byte so always a full write
          // If the last store matches this load value then we can replace the loaded value with the previous valid one
          IREmit->SetWriteCursor(CodeNode);
          IREmit->ReplaceAllUsesWith(CodeNode, LastNode);
          RecordAccess(Info, FEXCore::IR::GPRClass, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag, 1, ACCESS_READ, LastNode);
          Changed = true;
        }
        else if (IsReadAccess(LastAccess)) {
          IREmit->ReplaceAllUsesWith(CodeNode, LastNode);
          RecordAccess(Info, FEXCore::IR::GPRClass, offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag, 1, ACCESS_READ, LastNode);
          Changed = true;
        }
      }
      else if (IROp->Op == OP_SYSCALL ||
               IROp->Op == OP_INLINESYSCALL) {
        FEXCore::IR::SyscallFlags Flags{};
        if (IROp->Op == OP_SYSCALL) {
          auto Op = IROp->C<IR::IROp_Syscall>();
          Flags = Op->Flags;
        }
        else {
          auto Op = IROp->C<IR::IROp_InlineSyscall>();
          Flags = Op->Flags;
        }

        if ((Flags & FEXCore::IR::SyscallFlags::OPTIMIZETHROUGH) != FEXCore::IR::SyscallFlags::OPTIMIZETHROUGH) {
          // We can't track through these
          ResetClassificationAccesses(&LocalInfo, SupportsAVX);
        }
      }
      else if (IROp->Op == OP_STORECONTEXTINDEXED ||
               IROp->Op == OP_LOADCONTEXTINDEXED ||
               IROp->Op == OP_BREAK) {
        // We can't track through these
        ResetClassificationAccesses(&LocalInfo, SupportsAVX);
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

std::unique_ptr<FEXCore::IR::Pass> CreateContextLoadStoreElimination(bool SupportsAVX) {
  return std::make_unique<RCLSE>(SupportsAVX);
}

}
