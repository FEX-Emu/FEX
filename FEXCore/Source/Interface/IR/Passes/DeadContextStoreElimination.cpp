// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
desc: Transforms ContextLoad/Store to temporaries, similar to mem2reg
$end_info$
*/

#include "Interface/IR/IR.h"
#include "Interface/IR/IREmitter.h"
#include "Interface/IR/Passes.h"
#include "Interface/IR/PassManager.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/EnumOperators.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/vector.h>

#include <array>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <unordered_map>
#include <utility>

namespace {
struct ContextMemberClassification {
  size_t Offset;
  uint16_t Size;
};

enum class LastAccessType {
  NONE = (0b000 << 0),    ///< Was never previously accessed
  WRITE = (0b001 << 0),   ///< Was fully overwritten
  READ = (0b010 << 0),    ///< Was fully read
  INVALID = (0b011 << 0), ///< Accessing this is invalid
  MASK = (0b011 << 0),
  PARTIAL = (0b100 << 0),
  PARTIAL_WRITE = (PARTIAL | WRITE), ///< Was partially written
  PARTIAL_READ = (PARTIAL | READ),   ///< Was partially read
};
FEX_DEF_NUM_OPS(LastAccessType);

static bool IsWriteAccess(LastAccessType Type) {
  return (Type & LastAccessType::MASK) == LastAccessType::WRITE;
}

static bool IsReadAccess(LastAccessType Type) {
  return (Type & LastAccessType::MASK) == LastAccessType::READ;
}

[[maybe_unused]]
static bool IsInvalidAccess(LastAccessType Type) {
  return (Type & LastAccessType::MASK) == LastAccessType::INVALID;
}

[[maybe_unused]]
static bool IsPartialAccess(LastAccessType Type) {
  return (Type & LastAccessType::PARTIAL) == LastAccessType::PARTIAL;
}

[[maybe_unused]]
static bool IsFullAccess(LastAccessType Type) {
  return (Type & LastAccessType::PARTIAL) == LastAccessType::NONE;
}

struct ContextMemberInfo {
  ContextMemberClassification Class;
  LastAccessType Accessed;
  FEXCore::IR::RegisterClassType AccessRegClass;
  uint32_t AccessOffset;
  uint8_t AccessSize;
  ///< The last value that was loaded or stored.
  FEXCore::IR::OrderedNode* ValueNode;
  ///< With a store access, the store node that is doing the operation.
  FEXCore::IR::OrderedNode* StoreNode;
};

struct ContextInfo {
  fextl::vector<ContextMemberInfo*> Lookup;
  fextl::vector<ContextMemberInfo> ClassificationInfo;
};

static void ClassifyContextStruct(ContextInfo* ContextClassificationInfo, bool SupportsAVX) {
  auto ContextClassification = &ContextClassificationInfo->ClassificationInfo;

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, rip),
      sizeof(FEXCore::Core::CPUState::rip),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_GPRS; ++i) {
    ContextClassification->emplace_back(ContextMemberInfo {
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, gregs[0]) + sizeof(FEXCore::Core::CPUState::gregs[0]) * i,
        FEXCore::Core::CPUState::GPR_REG_SIZE,
      },
      LastAccessType::NONE,
      FEXCore::IR::InvalidClass,
    });
  }

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, es_idx),
      sizeof(FEXCore::Core::CPUState::es_idx),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, cs_idx),
      sizeof(FEXCore::Core::CPUState::cs_idx),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, ss_idx),
      sizeof(FEXCore::Core::CPUState::ss_idx),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, ds_idx),
      sizeof(FEXCore::Core::CPUState::ds_idx),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, gs_idx),
      sizeof(FEXCore::Core::CPUState::gs_idx),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, fs_idx),
      sizeof(FEXCore::Core::CPUState::fs_idx),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, _pad),
      sizeof(FEXCore::Core::CPUState::_pad),
    },
    LastAccessType::INVALID,
    FEXCore::IR::InvalidClass,
  });

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, es_cached),
      sizeof(FEXCore::Core::CPUState::es_cached),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, cs_cached),
      sizeof(FEXCore::Core::CPUState::cs_cached),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, ss_cached),
      sizeof(FEXCore::Core::CPUState::ss_cached),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, ds_cached),
      sizeof(FEXCore::Core::CPUState::ds_cached),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, gs_cached),
      sizeof(FEXCore::Core::CPUState::gs_cached),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, fs_cached),
      sizeof(FEXCore::Core::CPUState::fs_cached),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, InlineJITBlockHeader),
      sizeof(FEXCore::Core::CPUState::InlineJITBlockHeader),
    },
    LastAccessType::INVALID,
    FEXCore::IR::InvalidClass,
  });

  if (SupportsAVX) {
    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_XMMS; ++i) {
      ContextClassification->emplace_back(ContextMemberInfo {
        ContextMemberClassification {
          offsetof(FEXCore::Core::CPUState, xmm.avx.data[0][0]) + FEXCore::Core::CPUState::XMM_AVX_REG_SIZE * i,
          FEXCore::Core::CPUState::XMM_AVX_REG_SIZE,
        },
        LastAccessType::NONE,
        FEXCore::IR::InvalidClass,
      });
    }
  } else {
    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_XMMS; ++i) {
      ContextClassification->emplace_back(ContextMemberInfo {
        ContextMemberClassification {
          offsetof(FEXCore::Core::CPUState, xmm.sse.data[0][0]) + FEXCore::Core::CPUState::XMM_SSE_REG_SIZE * i,
          FEXCore::Core::CPUState::XMM_SSE_REG_SIZE,
        },
        LastAccessType::NONE,
        FEXCore::IR::InvalidClass,
      });
    }

    ContextClassification->emplace_back(ContextMemberInfo {
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, xmm.sse.pad[0][0]),
        static_cast<uint16_t>(FEXCore::Core::CPUState::XMM_SSE_REG_SIZE * FEXCore::Core::CPUState::NUM_XMMS),
      },
      LastAccessType::INVALID,
      FEXCore::IR::InvalidClass,
    });
  }

  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_FLAGS; ++i) {
    ContextClassification->emplace_back(ContextMemberInfo {
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, flags[0]) + sizeof(FEXCore::Core::CPUState::flags[0]) * i,
        FEXCore::Core::CPUState::FLAG_SIZE,
      },
      LastAccessType::NONE,
      FEXCore::IR::InvalidClass,
    });
  }

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, pf_raw),
      sizeof(FEXCore::Core::CPUState::pf_raw),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, af_raw),
      sizeof(FEXCore::Core::CPUState::af_raw),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_MMS; ++i) {
    ContextClassification->emplace_back(ContextMemberInfo {
      ContextMemberClassification {offsetof(FEXCore::Core::CPUState, mm[0][0]) + sizeof(FEXCore::Core::CPUState::mm[0]) * i,
                                   FEXCore::Core::CPUState::MM_REG_SIZE},
      LastAccessType::NONE,
      FEXCore::IR::InvalidClass,
    });
  }

  // GDTs
  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_GDTS; ++i) {
    ContextClassification->emplace_back(ContextMemberInfo {
      ContextMemberClassification {
        offsetof(FEXCore::Core::CPUState, gdt[0]) + sizeof(FEXCore::Core::CPUState::gdt[0]) * i,
        sizeof(FEXCore::Core::CPUState::gdt[0]),
      },
      LastAccessType::NONE,
      FEXCore::IR::InvalidClass,
    });
  }

  // FCW
  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, FCW),
      sizeof(FEXCore::Core::CPUState::FCW),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  // AbridgedFTW
  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, AbridgedFTW),
      sizeof(FEXCore::Core::CPUState::AbridgedFTW),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  // _pad2
  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, _pad2),
      sizeof(FEXCore::Core::CPUState::_pad2),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  // DeferredSignalRefCount
  ContextClassification->emplace_back(ContextMemberInfo {
    ContextMemberClassification {
      offsetof(FEXCore::Core::CPUState, DeferredSignalRefCount),
      sizeof(FEXCore::Core::CPUState::DeferredSignalRefCount),
    },
    LastAccessType::NONE,
    FEXCore::IR::InvalidClass,
  });

  [[maybe_unused]] size_t ClassifiedStructSize {};
  ContextClassificationInfo->Lookup.reserve(sizeof(FEXCore::Core::CPUState));
  for (auto& it : *ContextClassification) {
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

static void ResetClassificationAccesses(ContextInfo* ContextClassificationInfo, bool SupportsAVX) {
  auto ContextClassification = &ContextClassificationInfo->ClassificationInfo;

  auto SetAccess = [&](size_t Offset, LastAccessType Access) {
    ContextClassification->at(Offset).Accessed = Access;
    ContextClassification->at(Offset).AccessRegClass = FEXCore::IR::InvalidClass;
    ContextClassification->at(Offset).AccessOffset = 0;
    ContextClassification->at(Offset).StoreNode = nullptr;
  };
  size_t Offset = 0;
  SetAccess(Offset++, LastAccessType::NONE);
  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_GPRS; ++i) {
    SetAccess(Offset++, LastAccessType::NONE);
  }

  // Segment indexes
  SetAccess(Offset++, LastAccessType::NONE);
  SetAccess(Offset++, LastAccessType::NONE);
  SetAccess(Offset++, LastAccessType::NONE);
  SetAccess(Offset++, LastAccessType::NONE);
  SetAccess(Offset++, LastAccessType::NONE);
  SetAccess(Offset++, LastAccessType::NONE);

  // Pad
  SetAccess(Offset++, LastAccessType::INVALID);

  // Segments
  SetAccess(Offset++, LastAccessType::NONE);
  SetAccess(Offset++, LastAccessType::NONE);
  SetAccess(Offset++, LastAccessType::NONE);
  SetAccess(Offset++, LastAccessType::NONE);
  SetAccess(Offset++, LastAccessType::NONE);
  SetAccess(Offset++, LastAccessType::NONE);

  // Pad2
  SetAccess(Offset++, LastAccessType::INVALID);

  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_XMMS; ++i) {
    SetAccess(Offset++, LastAccessType::NONE);
  }

  if (!SupportsAVX) {
    SetAccess(Offset++, LastAccessType::NONE);
  }

  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_FLAGS; ++i) {
    SetAccess(Offset++, LastAccessType::NONE);
  }

  // PF/AF
  SetAccess(Offset++, LastAccessType::NONE);
  SetAccess(Offset++, LastAccessType::NONE);

  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_MMS; ++i) {
    SetAccess(Offset++, LastAccessType::NONE);
  }

  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_GDTS; ++i) {
    SetAccess(Offset++, LastAccessType::NONE);
  }

  SetAccess(Offset++, LastAccessType::NONE);
  SetAccess(Offset++, LastAccessType::NONE);

  SetAccess(Offset++, LastAccessType::INVALID);
  SetAccess(Offset++, LastAccessType::INVALID);
}

struct BlockInfo {
  fextl::vector<FEXCore::IR::OrderedNode*> Predecessors;
  fextl::vector<FEXCore::IR::OrderedNode*> Successors;
  ContextInfo IncomingClassifiedStruct;
  ContextInfo OutgoingClassifiedStruct;
};

class RCLSE final : public FEXCore::IR::Pass {
public:
  explicit RCLSE(bool SupportsAVX_)
    : SupportsAVX {SupportsAVX_} {
    ClassifyContextStruct(&ClassifiedStruct, SupportsAVX);
    DCE = FEXCore::IR::CreatePassDeadCodeElimination();
  }
  void Run(FEXCore::IR::IREmitter* IREmit) override;
private:
  fextl::unique_ptr<FEXCore::IR::Pass> DCE;

  ContextInfo ClassifiedStruct;
  fextl::unordered_map<FEXCore::IR::NodeID, BlockInfo> OffsetToBlockMap;

  bool SupportsAVX;

  ContextMemberInfo* FindMemberInfo(ContextInfo* ClassifiedInfo, uint32_t Offset, uint8_t Size);
  ContextMemberInfo* RecordAccess(ContextMemberInfo* Info, FEXCore::IR::RegisterClassType RegClass, uint32_t Offset, uint8_t Size,
                                  LastAccessType AccessType, FEXCore::IR::OrderedNode* Node, FEXCore::IR::OrderedNode* StoreNode = nullptr);
  ContextMemberInfo* RecordAccess(ContextInfo* ClassifiedInfo, FEXCore::IR::RegisterClassType RegClass, uint32_t Offset, uint8_t Size,
                                  LastAccessType AccessType, FEXCore::IR::OrderedNode* Node, FEXCore::IR::OrderedNode* StoreNode = nullptr);

  void HandleLoadFlag(FEXCore::IR::IREmitter* IREmit, ContextInfo* LocalInfo, FEXCore::IR::OrderedNode* CodeNode, unsigned Flag);

  // Classify context loads and stores.
  void ClassifyContextLoad(FEXCore::IR::IREmitter* IREmit, ContextInfo* LocalInfo, FEXCore::IR::RegisterClassType Class, uint32_t Offset,
                           uint8_t Size, FEXCore::IR::OrderedNode* CodeNode, FEXCore::IR::NodeIterator BlockEnd);
  void ClassifyContextStore(FEXCore::IR::IREmitter* IREmit, ContextInfo* LocalInfo, FEXCore::IR::RegisterClassType Class, uint32_t Offset,
                            uint8_t Size, FEXCore::IR::OrderedNode* CodeNode, FEXCore::IR::OrderedNode* ValueNode);

  // Block local Passes
  void RedundantStoreLoadElimination(FEXCore::IR::IREmitter* IREmit);

  unsigned OffsetForReg(FEXCore::IR::RegisterClassType Class, unsigned Reg, unsigned Size) {
    if (Class == FEXCore::IR::FPRClass) {
      return Size == 32 ? offsetof(FEXCore::Core::CPUState, xmm.avx.data[Reg][0]) : offsetof(FEXCore::Core::CPUState, xmm.sse.data[Reg][0]);
    } else if (Reg == FEXCore::Core::CPUState::PF_AS_GREG) {
      return offsetof(FEXCore::Core::CPUState, pf_raw);
    } else if (Reg == FEXCore::Core::CPUState::AF_AS_GREG) {
      return offsetof(FEXCore::Core::CPUState, af_raw);
    } else {
      return offsetof(FEXCore::Core::CPUState, gregs[Reg]);
    }
  }
};

ContextMemberInfo* RCLSE::FindMemberInfo(ContextInfo* ContextClassificationInfo, uint32_t Offset, uint8_t Size) {
  return ContextClassificationInfo->Lookup.at(Offset);
}

ContextMemberInfo* RCLSE::RecordAccess(ContextMemberInfo* Info, FEXCore::IR::RegisterClassType RegClass, uint32_t Offset, uint8_t Size,
                                       LastAccessType AccessType, FEXCore::IR::OrderedNode* ValueNode, FEXCore::IR::OrderedNode* StoreNode) {
  LOGMAN_THROW_AA_FMT((Offset + Size) <= (Info->Class.Offset + Info->Class.Size), "Access to context item went over member size");
  LOGMAN_THROW_AA_FMT(Info->Accessed != LastAccessType::INVALID, "Tried to access invalid member");

  // If we aren't fully overwriting the member then it is a partial write that we need to track
  if (Size < Info->Class.Size) {
    AccessType = AccessType == LastAccessType::WRITE ? LastAccessType::PARTIAL_WRITE : LastAccessType::PARTIAL_READ;
  }
  if (Size > Info->Class.Size) {
    LOGMAN_MSG_A_FMT("Can't handle this");
  }

  Info->Accessed = AccessType;
  Info->AccessRegClass = RegClass;
  Info->AccessOffset = Offset;
  Info->AccessSize = Size;
  Info->ValueNode = ValueNode;
  if (StoreNode != nullptr) {
    Info->StoreNode = StoreNode;
  }
  return Info;
}

ContextMemberInfo* RCLSE::RecordAccess(ContextInfo* ClassifiedInfo, FEXCore::IR::RegisterClassType RegClass, uint32_t Offset, uint8_t Size,
                                       LastAccessType AccessType, FEXCore::IR::OrderedNode* ValueNode, FEXCore::IR::OrderedNode* StoreNode) {
  ContextMemberInfo* Info = FindMemberInfo(ClassifiedInfo, Offset, Size);
  return RecordAccess(Info, RegClass, Offset, Size, AccessType, ValueNode, StoreNode);
}

void RCLSE::ClassifyContextLoad(FEXCore::IR::IREmitter* IREmit, ContextInfo* LocalInfo, FEXCore::IR::RegisterClassType Class,
                                uint32_t Offset, uint8_t Size, FEXCore::IR::OrderedNode* CodeNode, FEXCore::IR::NodeIterator BlockEnd) {
  auto Info = FindMemberInfo(LocalInfo, Offset, Size);
  ContextMemberInfo PreviousMemberInfoCopy = *Info;
  RecordAccess(Info, Class, Offset, Size, LastAccessType::READ, CodeNode);

  if (PreviousMemberInfoCopy.AccessRegClass == Info->AccessRegClass && PreviousMemberInfoCopy.AccessOffset == Info->AccessOffset &&
      PreviousMemberInfoCopy.AccessSize == Size) {
    // This optimizes two cases:
    // - Previous access was a load, and we have a redundant load of the same value.
    // - Previous access was a store, and we are redundantly loading immediately after the store. Eliminating the store.
    IREmit->ReplaceAllUsesWithRange(CodeNode, PreviousMemberInfoCopy.ValueNode, IREmit->GetIterator(IREmit->WrapNode(CodeNode)), BlockEnd);
    RecordAccess(Info, Class, Offset, Size, LastAccessType::READ, PreviousMemberInfoCopy.ValueNode);
  }
  // TODO: Optimize the case of partial loads.
}

void RCLSE::ClassifyContextStore(FEXCore::IR::IREmitter* IREmit, ContextInfo* LocalInfo, FEXCore::IR::RegisterClassType Class,
                                 uint32_t Offset, uint8_t Size, FEXCore::IR::OrderedNode* CodeNode, FEXCore::IR::OrderedNode* ValueNode) {
  auto Info = FindMemberInfo(LocalInfo, Offset, Size);
  ContextMemberInfo PreviousMemberInfoCopy = *Info;
  RecordAccess(Info, Class, Offset, Size, LastAccessType::WRITE, ValueNode, CodeNode);

  if (PreviousMemberInfoCopy.AccessRegClass == Info->AccessRegClass && PreviousMemberInfoCopy.AccessOffset == Info->AccessOffset &&
      PreviousMemberInfoCopy.AccessSize == Size && PreviousMemberInfoCopy.Accessed == LastAccessType::WRITE) {
    // This optimizes redundant stores with no intervening load
    // TODO: this is causing RA to fall over in some titles, disabling for now.
    // Revisit when the new RA lands.
#if 0
    IREmit->Remove(PreviousMemberInfoCopy.StoreNode);
#endif
  }

  // TODO: Optimize the case of partial stores.
}

void RCLSE::HandleLoadFlag(FEXCore::IR::IREmitter* IREmit, ContextInfo* LocalInfo, FEXCore::IR::OrderedNode* CodeNode, unsigned Flag) {
  const auto FlagOffset = offsetof(FEXCore::Core::CPUState, flags[Flag]);
  auto Info = FindMemberInfo(LocalInfo, FlagOffset, 1);
  LastAccessType LastAccess = Info->Accessed;
  auto LastValueNode = Info->ValueNode;

  if (IsWriteAccess(LastAccess)) { // 1 byte so always a full write
    // If the last store matches this load value then we can replace the loaded value with the previous valid one
    IREmit->SetWriteCursor(CodeNode);
    IREmit->ReplaceAllUsesWith(CodeNode, LastValueNode);
    RecordAccess(Info, FEXCore::IR::GPRClass, FlagOffset, 1, LastAccessType::READ, LastValueNode);
  } else if (IsReadAccess(LastAccess)) {
    IREmit->ReplaceAllUsesWith(CodeNode, LastValueNode);
    RecordAccess(Info, FEXCore::IR::GPRClass, FlagOffset, 1, LastAccessType::READ, LastValueNode);
  }
}

/**
 * @brief This pass removes redundant pairs of storecontext and loadcontext ops
 *
 * eg.
 *   %26 i128 = LoadMem %25 i64, 0x10
 *   (%%27) StoreContext %26 i128, 0x10, 0xb0
 *   %28 i128 = LoadContext 0x10, 0x90
 *   %29 i128 = LoadContext 0x10, 0xb0
 * Converts to
 *   %26 i128 = LoadMem %25 i64, 0x10
 *   (%%27) StoreContext %26 i128, 0x10, 0xb0
 *   %28 i128 = LoadContext 0x10, 0x90
 *
 * eg.
 * 		%6 i128 = LoadContext 0x10, 0x90
 *		%7 i128 = LoadContext 0x10, 0x90
 *		%8 i128 = VXor %7 i128, %6 i128
 * Converts to
 *    %6 i128 = LoadContext 0x10, 0x90
 *    %7 i128 = VXor %6 i128, %6 i128
 *
 * eg.
 *   (%%189) StoreContext %188 i128, 0x10, 0xa0
 *   %190 i128 = LoadContext 0x10, 0x90
 *   %192 i128 = VAdd %188 i128, %190 i128, 0x10, 0x4
 *   (%%193) StoreContext %192 i128, 0x10, 0xa0
 * Converts to
 *   %173 i128 = LoadContext 0x10, 0x90
 *   %175 i128 = VAdd %172 i128, %173 i128, 0x10, 0x4
 *   (%%176) StoreContext %175 i128, 0x10, 0xa0

 */
void RCLSE::RedundantStoreLoadElimination(FEXCore::IR::IREmitter* IREmit) {
  using namespace FEXCore;
  using namespace FEXCore::IR;

  auto CurrentIR = IREmit->ViewIR();
  auto OriginalWriteCursor = IREmit->GetWriteCursor();

  // XXX: Walk the list and calculate the control flow

  ContextInfo& LocalInfo = ClassifiedStruct;

  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
    auto BlockOp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();
    auto BlockEnd = IREmit->GetIterator(BlockOp->Last);

    ResetClassificationAccesses(&LocalInfo, SupportsAVX);

    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      if (IROp->Op == OP_STORECONTEXT) {
        auto Op = IROp->CW<IR::IROp_StoreContext>();
        ClassifyContextStore(IREmit, &LocalInfo, Op->Class, Op->Offset, IROp->Size, CodeNode, CurrentIR.GetNode(Op->Value));
      } else if (IROp->Op == OP_STOREREGISTER) {
        auto Op = IROp->CW<IR::IROp_StoreRegister>();
        auto Offset = OffsetForReg(Op->Class, Op->Reg, IROp->Size);

        ClassifyContextStore(IREmit, &LocalInfo, Op->Class, Offset, IROp->Size, CodeNode, CurrentIR.GetNode(Op->Value));
      } else if (IROp->Op == OP_LOADREGISTER) {
        auto Op = IROp->CW<IR::IROp_LoadRegister>();
        auto Offset = OffsetForReg(Op->Class, Op->Reg, IROp->Size);
        ClassifyContextLoad(IREmit, &LocalInfo, Op->Class, Offset, IROp->Size, CodeNode, BlockEnd);
      } else if (IROp->Op == OP_LOADCONTEXT) {
        auto Op = IROp->CW<IR::IROp_LoadContext>();
        ClassifyContextLoad(IREmit, &LocalInfo, Op->Class, Op->Offset, IROp->Size, CodeNode, BlockEnd);
      } else if (IROp->Op == OP_STOREFLAG) {
        const auto Op = IROp->CW<IR::IROp_StoreFlag>();
        const auto FlagOffset = offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag;
        auto Info = FindMemberInfo(&LocalInfo, FlagOffset, 1);
        auto LastStoreNode = Info->StoreNode;
        RecordAccess(&LocalInfo, FEXCore::IR::GPRClass, FlagOffset, 1, LastAccessType::WRITE, CurrentIR.GetNode(Op->Header.Args[0]), CodeNode);

        // Flags don't alias, so we can take the simple route here. Kill any flags that have been overwritten
        if (LastStoreNode != nullptr) {
          IREmit->Remove(LastStoreNode);
        }
      } else if (IROp->Op == OP_INVALIDATEFLAGS) {
        auto Op = IROp->CW<IR::IROp_InvalidateFlags>();

        // Loop through non-reserved flag stores and eliminate unused ones.
        for (size_t F = 0; F < Core::CPUState::NUM_EFLAG_BITS; F++) {
          if (!(Op->Flags & (1ULL << F))) {
            continue;
          }

          const auto FlagOffset = offsetof(FEXCore::Core::CPUState, flags[0]) + F;
          auto Info = FindMemberInfo(&LocalInfo, FlagOffset, 1);
          auto LastStoreNode = Info->StoreNode;

          // Flags don't alias, so we can take the simple route here. Kill any flags that have been invalidated without a read.
          if (LastStoreNode != nullptr) {
            IREmit->SetWriteCursor(CodeNode);
            RecordAccess(&LocalInfo, FEXCore::IR::GPRClass, FlagOffset, 1, LastAccessType::WRITE, IREmit->_Constant(0), CodeNode);

            IREmit->Remove(LastStoreNode);
          }
        }
      } else if (IROp->Op == OP_LOADFLAG) {
        const auto Op = IROp->CW<IR::IROp_LoadFlag>();

        HandleLoadFlag(IREmit, &LocalInfo, CodeNode, Op->Flag);
      } else if (IROp->Op == OP_LOADDF) {
        HandleLoadFlag(IREmit, &LocalInfo, CodeNode, X86State::RFLAG_DF_RAW_LOC);
      } else if (IROp->Op == OP_SYSCALL || IROp->Op == OP_INLINESYSCALL) {
        FEXCore::IR::SyscallFlags Flags {};
        if (IROp->Op == OP_SYSCALL) {
          auto Op = IROp->C<IR::IROp_Syscall>();
          Flags = Op->Flags;
        } else {
          auto Op = IROp->C<IR::IROp_InlineSyscall>();
          Flags = Op->Flags;
        }

        if ((Flags & FEXCore::IR::SyscallFlags::OPTIMIZETHROUGH) != FEXCore::IR::SyscallFlags::OPTIMIZETHROUGH) {
          // We can't track through these
          ResetClassificationAccesses(&LocalInfo, SupportsAVX);
        }
      } else if (IROp->Op == OP_STORECONTEXTINDEXED || IROp->Op == OP_LOADCONTEXTINDEXED || IROp->Op == OP_BREAK) {
        // We can't track through these
        ResetClassificationAccesses(&LocalInfo, SupportsAVX);
      }
    }
  }

  IREmit->SetWriteCursor(OriginalWriteCursor);
}

void RCLSE::Run(FEXCore::IR::IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::RCLSE");
  RedundantStoreLoadElimination(IREmit);
}

} // namespace

namespace FEXCore::IR {

fextl::unique_ptr<FEXCore::IR::Pass> CreateContextLoadStoreElimination(bool SupportsAVX) {
  return fextl::make_unique<RCLSE>(SupportsAVX);
}

} // namespace FEXCore::IR
