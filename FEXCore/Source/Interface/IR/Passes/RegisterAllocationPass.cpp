// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
$end_info$
*/

#include "Interface/IR/Passes/RegisterAllocationPass.h"
#include "Interface/IR/Passes.h"
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IREmitter.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/RegisterAllocationData.h>
#include <FEXCore/Utils/BitUtils.h>
#include <FEXCore/Utils/BucketList.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/set.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/unordered_set.h>
#include <FEXCore/fextl/vector.h>

#include <FEXHeaderUtils/TypeDefines.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <strings.h>
#include <utility>

#define SRA_DEBUG(...) // fextl::fmt::print(__VA_ARGS__)

namespace FEXCore::IR {
namespace {
  constexpr uint32_t INVALID_REG = FEXCore::IR::InvalidReg;
  constexpr uint32_t INVALID_CLASS = FEXCore::IR::InvalidClass.Val;

  constexpr uint32_t DEFAULT_INTERFERENCE_LIST_COUNT = 122;
  constexpr uint32_t DEFAULT_INTERFERENCE_SPAN_COUNT = 30;
  constexpr uint32_t DEFAULT_NODE_COUNT = 8192;

  struct Register {
    bool Virtual;
    uint64_t Index;
  };

  struct RegisterClass {
    uint32_t CountMask;
    uint32_t PhysicalCount;
  };

  struct RegisterNode {
    struct VolatileHeader {
      IR::NodeID BlockID{UINT32_MAX};
      uint32_t SpillSlot{UINT32_MAX};
      uint64_t Padding;
    };

    VolatileHeader Head;
    FEXCore::BucketList<DEFAULT_INTERFERENCE_LIST_COUNT, IR::NodeID> Interferences;
  };

  static_assert(sizeof(RegisterNode) == 128 * 4);
  constexpr size_t REGISTER_NODES_PER_PAGE = FHU::FEX_PAGE_SIZE / sizeof(RegisterNode);

  struct RegisterSet {
    fextl::vector<RegisterClass> Classes;
    uint32_t ClassCount;
    uint32_t Conflicts[ 8 * 8 * 32 * 32];
  };

  struct LiveRange {
    IR::NodeID Begin{UINT32_MAX};
    IR::NodeID End{UINT32_MAX};
    uint32_t RematCost{0};
    IR::NodeID PreWritten{0};
    PhysicalRegister PrefferedRegister{PhysicalRegister::Invalid()};
    bool Written{false};
    bool Global{false};
  };

  struct SpillStackUnit {
    IR::NodeID Node;
    IR::RegisterClassType Class;
    LiveRange SpillRange;
    IR::OrderedNode *SpilledNode;
  };

  struct RegisterGraph : public FEXCore::Allocator::FEXAllocOperators {
    IR::RegisterAllocationData::UniquePtr AllocData;
    RegisterSet Set;
    fextl::vector<RegisterNode> Nodes{};
    uint32_t NodeCount{};
    fextl::vector<SpillStackUnit> SpillStack;
    fextl::unordered_map<IR::NodeID, fextl::unordered_set<IR::NodeID>> BlockPredecessors;
    fextl::unordered_map<IR::NodeID, fextl::unordered_set<IR::NodeID>> VisitedNodePredecessors;
  };

  void ResetRegisterGraph(RegisterGraph *Graph, uint64_t NodeCount);

  RegisterGraph *AllocateRegisterGraph(uint32_t ClassCount) {
    RegisterGraph *Graph = new RegisterGraph{};

    // Allocate the register set
    Graph->Set.ClassCount = ClassCount;
    Graph->Set.Classes.resize(ClassCount);

    // Allocate default nodes
    ResetRegisterGraph(Graph, DEFAULT_NODE_COUNT);
    return Graph;
  }


  void AllocatePhysicalRegisters(RegisterGraph *Graph, FEXCore::IR::RegisterClassType Class, uint32_t Count) {
    Graph->Set.Classes[Class].CountMask = (1 << Count) - 1;
    Graph->Set.Classes[Class].PhysicalCount = Count;
  }

  void SetConflict(RegisterGraph *Graph, PhysicalRegister RegAndClass, PhysicalRegister ConflictRegAndClass) {
    uint32_t Index = (ConflictRegAndClass.Class << 8) | RegAndClass.Raw;

    Graph->Set.Conflicts[Index] |= 1 << ConflictRegAndClass.Reg;
  }

  uint32_t GetConflicts(RegisterGraph *Graph, PhysicalRegister RegAndClass, FEXCore::IR::RegisterClassType ConflictClass) {
    uint32_t Index = (ConflictClass.Val << 8) | RegAndClass.Raw;

    return Graph->Set.Conflicts[Index];
  }

  void VirtualAddRegisterConflict(RegisterGraph *Graph, FEXCore::IR::RegisterClassType ClassConflict, uint32_t RegConflict, FEXCore::IR::RegisterClassType Class, uint32_t Reg) {

    auto RegAndClass = PhysicalRegister(Class, Reg);
    auto RegAndClassConflict = PhysicalRegister(ClassConflict, RegConflict);

    // Conflict must go both ways
    SetConflict(Graph, RegAndClass, RegAndClassConflict);
    SetConflict(Graph, RegAndClassConflict, RegAndClass);
  }

  void FreeRegisterGraph(RegisterGraph *Graph) {
    delete Graph;
  }

  void ResetRegisterGraph(RegisterGraph *Graph, uint64_t NodeCount) {
    NodeCount = FEXCore::AlignUp(NodeCount, REGISTER_NODES_PER_PAGE);

    // Clear to free the Bucketlists which have unique_ptrs
    // Resize to our correct size
    Graph->Nodes.clear();
    Graph->Nodes.resize(NodeCount);

    Graph->VisitedNodePredecessors.clear();
    Graph->AllocData = RegisterAllocationData::Create(NodeCount);
    Graph->NodeCount = NodeCount;
  }

  void SetNodeClass(RegisterGraph *Graph, IR::NodeID Node, FEXCore::IR::RegisterClassType Class) {
    Graph->AllocData->Map[Node.Value].Class = Class.Val;
  }

  FEXCore::IR::RegisterClassType GetRegClassFromNode(FEXCore::IR::IRListView *IR, FEXCore::IR::IROp_Header *IROp) {
    using namespace FEXCore;

    FEXCore::IR::RegisterClassType Class = IR::GetRegClass(IROp->Op);
    if (Class != FEXCore::IR::ComplexClass)
      return Class;

    // Complex register class handling
    switch (IROp->Op) {
      case IR::OP_LOADCONTEXT: {
        auto Op = IROp->C<IR::IROp_LoadContext>();
        return Op->Class;
        break;
      }
      case IR::OP_LOADREGISTER: {
        auto Op = IROp->C<IR::IROp_LoadRegister>();
        return Op->Class;
        break;
      }
      case IR::OP_LOADCONTEXTINDEXED: {
        auto Op = IROp->C<IR::IROp_LoadContextIndexed>();
        return Op->Class;
        break;
      }
      case IR::OP_LOADMEM:
      case IR::OP_LOADMEMTSO: {
        auto Op = IROp->C<IR::IROp_LoadMem>();
        return Op->Class;
        break;
      }
      case IR::OP_FILLREGISTER: {
        auto Op = IROp->C<IR::IROp_FillRegister>();
        return Op->Class;
        break;
      }
      default: break;
    }

    // Unreachable
    return FEXCore::IR::InvalidClass;
  };

  // Walk the IR and set the node classes
  void FindNodeClasses(RegisterGraph *Graph, FEXCore::IR::IRListView *IR) {
    for (auto [CodeNode, IROp] : IR->GetAllCode()) {
      // If the destination hasn't yet been set then set it now
      if (GetHasDest(IROp->Op)) {
        const auto ID = IR->GetID(CodeNode);
        Graph->AllocData->Map[ID.Value] = PhysicalRegister(GetRegClassFromNode(IR, IROp), INVALID_REG);
      } else {
        //Graph->AllocData->Map[IR->GetID(CodeNode)] = PhysicalRegister::Invalid();
      }
    }
  }
} // Anonymous namespace

  class ConstrainedRAPass final : public RegisterAllocationPass {
    public:
      ConstrainedRAPass(FEXCore::IR::Pass* _CompactionPass, bool OptimizeSRA, bool SupportsAVX);
      ~ConstrainedRAPass();
      bool Run(IREmitter *IREmit) override;

      void AllocateRegisterSet(uint32_t ClassCount) override;
      void AddRegisters(FEXCore::IR::RegisterClassType Class, uint32_t RegisterCount) override;
      void AddRegisterConflict(FEXCore::IR::RegisterClassType ClassConflict, uint32_t RegConflict, FEXCore::IR::RegisterClassType Class, uint32_t Reg) override;

      /**
       * @brief Returns the register and class encoded together
       * Top 32bits is the class, lower 32bits is the register
       */
      RegisterAllocationData* GetAllocationData() override;
      RegisterAllocationData::UniquePtr PullAllocationData() override;

    private:
      using BlockInterferences = fextl::vector<IR::NodeID>;

      IR::NodeID SpillPointId;

      fextl::vector<BucketList<DEFAULT_INTERFERENCE_SPAN_COUNT, uint32_t>> SpanStart;
      fextl::vector<BucketList<DEFAULT_INTERFERENCE_SPAN_COUNT, uint32_t>> SpanEnd;

      RegisterGraph *Graph;
      FEXCore::IR::Pass* CompactionPass;
      bool OptimizeSRA;
      bool SupportsAVX;

      fextl::vector<LiveRange> LiveRanges;

      fextl::unordered_map<IR::NodeID, BlockInterferences> LocalBlockInterferences;
      BlockInterferences GlobalBlockInterferences;

      [[nodiscard]] static constexpr uint32_t InfoMake(uint32_t id, uint32_t Class) {
        return id | (Class << 24);
      }
      [[nodiscard]] static constexpr uint32_t InfoIDClass(uint32_t info) {
        return info & 0xffff'ffff;
      }
      [[nodiscard]] static constexpr IR::NodeID InfoID(uint32_t info) {
        return IR::NodeID{info & 0xff'ffff};
      }
      [[nodiscard]] static constexpr uint32_t InfoClass(uint32_t info) {
        return info & 0xff00'0000;
      }

      void SpillOne(FEXCore::IR::IREmitter *IREmit);

      void CalculateLiveRange(FEXCore::IR::IRListView *IR);
      void OptimizeStaticRegisters(FEXCore::IR::IRListView *IR);
      void CalculateBlockInterferences(FEXCore::IR::IRListView *IR);
      void CalculateBlockNodeInterference(FEXCore::IR::IRListView *IR);
      void CalculateNodeInterference(FEXCore::IR::IRListView *IR);
      void AllocateVirtualRegisters();
      void CalculatePredecessors(FEXCore::IR::IRListView *IR);
      void RecursiveLiveRangeExpansion(FEXCore::IR::IRListView *IR,
                                       IR::NodeID Node, IR::NodeID DefiningBlockID,
                                       LiveRange *LiveRange,
                                       const fextl::unordered_set<IR::NodeID> &Predecessors,
                                       fextl::unordered_set<IR::NodeID> &VisitedPredecessors);

      FEXCore::IR::AllNodesIterator FindFirstUse(FEXCore::IR::IREmitter *IREmit, FEXCore::IR::OrderedNode* Node, FEXCore::IR::AllNodesIterator Begin, FEXCore::IR::AllNodesIterator End);
      FEXCore::IR::AllNodesIterator FindLastUseBefore(FEXCore::IR::IREmitter *IREmit, FEXCore::IR::OrderedNode* Node, FEXCore::IR::AllNodesIterator Begin, FEXCore::IR::AllNodesIterator End);

      std::optional<IR::NodeID> FindNodeToSpill(IREmitter *IREmit,
                                                RegisterNode *RegisterNode,
                                                IR::NodeID CurrentLocation,
                                                LiveRange const *OpLiveRange,
                                                int32_t RematCost = -1);
      uint32_t FindSpillSlot(IR::NodeID Node, FEXCore::IR::RegisterClassType RegisterClass);

      bool RunAllocateVirtualRegisters(IREmitter *IREmit);
  };

  ConstrainedRAPass::ConstrainedRAPass(FEXCore::IR::Pass* _CompactionPass, bool _OptimizeSRA, bool _SupportsAVX)
    : CompactionPass {_CompactionPass}, OptimizeSRA(_OptimizeSRA), SupportsAVX{_SupportsAVX} {
  }

  ConstrainedRAPass::~ConstrainedRAPass() {
    FreeRegisterGraph(Graph);
  }

  void ConstrainedRAPass::AllocateRegisterSet(uint32_t ClassCount) {
    LOGMAN_THROW_AA_FMT(ClassCount <= INVALID_CLASS, "Up to {} classes supported", INVALID_CLASS);

    Graph = AllocateRegisterGraph(ClassCount);

    // Add identity conflicts
    for (uint32_t Class = 0; Class < INVALID_CLASS; Class++) {
      for (uint32_t Reg = 0; Reg < INVALID_REG; Reg++) {
        AddRegisterConflict(RegisterClassType{Class}, Reg, RegisterClassType{Class}, Reg);
      }
    }
  }

  void ConstrainedRAPass::AddRegisters(FEXCore::IR::RegisterClassType Class, uint32_t RegisterCount) {
    LOGMAN_THROW_AA_FMT(RegisterCount <= INVALID_REG, "Up to {} regs supported", INVALID_REG);

    AllocatePhysicalRegisters(Graph, Class, RegisterCount);
  }

  void ConstrainedRAPass::AddRegisterConflict(FEXCore::IR::RegisterClassType ClassConflict, uint32_t RegConflict, FEXCore::IR::RegisterClassType Class, uint32_t Reg) {
    VirtualAddRegisterConflict(Graph, ClassConflict, RegConflict, Class, Reg);
  }

  RegisterAllocationData* ConstrainedRAPass::GetAllocationData() {
    return Graph->AllocData.get();
  }

  RegisterAllocationData::UniquePtr ConstrainedRAPass::PullAllocationData() {
    return std::move(Graph->AllocData);
  }

  void ConstrainedRAPass::RecursiveLiveRangeExpansion(IR::IRListView *IR,
                                                      IR::NodeID Node, IR::NodeID DefiningBlockID,
                                                      LiveRange *LiveRange,
                                                      const fextl::unordered_set<IR::NodeID> &Predecessors,
                                                      fextl::unordered_set<IR::NodeID> &VisitedPredecessors) {
    for (auto PredecessorId: Predecessors) {
      if (DefiningBlockID != PredecessorId && !VisitedPredecessors.contains(PredecessorId)) {
        // do the magic
        VisitedPredecessors.insert(PredecessorId);

        auto [_, IROp] = *IR->at(PredecessorId);

        auto Op = IROp->C<IROp_CodeBlock>();
        const auto BeginID = Op->Begin.ID();
        const auto LastID = Op->Last.ID();

        LOGMAN_THROW_AA_FMT(Op->Header.Op == OP_CODEBLOCK, "Block not defined by codeblock?");

        LiveRange->Begin = std::min(LiveRange->Begin, BeginID);
        LiveRange->End = std::max(LiveRange->End, BeginID);

        LiveRange->Begin = std::min(LiveRange->Begin, LastID);
        LiveRange->End = std::max(LiveRange->End, LastID);

        RecursiveLiveRangeExpansion(IR, Node, DefiningBlockID, LiveRange,
                                    Graph->BlockPredecessors[PredecessorId],
                                    VisitedPredecessors);
      }
    }
  }

  [[nodiscard]] static uint32_t CalculateRematCost(IROps Op) {
    constexpr uint32_t DEFAULT_REMAT_COST = 1000;

    switch (Op) {
      case IR::OP_CONSTANT:
        return 1;

      case IR::OP_LOADFLAG:
      case IR::OP_LOADCONTEXT:
      case IR::OP_LOADREGISTER:
        return 10;

      case IR::OP_LOADMEM:
      case IR::OP_LOADMEMTSO:
        return 100;

      case IR::OP_FILLREGISTER:
        return DEFAULT_REMAT_COST + 1;

      default:
        return DEFAULT_REMAT_COST;
    }
  }

  void ConstrainedRAPass::CalculateLiveRange(FEXCore::IR::IRListView *IR) {
    using namespace FEXCore;
    size_t Nodes = IR->GetSSACount();
    LiveRanges.clear();
    LiveRanges.resize(Nodes);

    for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
      const auto BlockNodeID = IR->GetID(BlockNode);
      for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
        const auto Node = IR->GetID(CodeNode);
        auto& NodeLiveRange = LiveRanges[Node.Value];

        // If the destination hasn't yet been set then set it now
        if (GetHasDest(IROp->Op)) {
          LOGMAN_THROW_AA_FMT(NodeLiveRange.Begin.Value == UINT32_MAX,
                             "Node begin already defined?");
          NodeLiveRange.Begin = Node;
          // Default to ending right where after it starts
          NodeLiveRange.End = IR::NodeID{Node.Value + 1};
        }

        // Calculate remat cost
        NodeLiveRange.RematCost = CalculateRematCost(IROp->Op);

        // Set this node's block ID
        Graph->Nodes[Node.Value].Head.BlockID = BlockNodeID;

        // FillRegister's SSA arg is only there for verification, and we don't want it
        // to impact the live range.
        if (IROp->Op == OP_FILLREGISTER) {
          continue;
        }

        const uint8_t NumArgs = IR::GetRAArgs(IROp->Op);
        for (uint8_t i = 0; i < NumArgs; ++i) {
          const auto& Arg = IROp->Args[i];

          if (Arg.IsInvalid()) {
            continue;
          }
          if (IR->GetOp<IROp_Header>(Arg)->Op == OP_INLINECONSTANT) {
            continue;
          }
          if (IR->GetOp<IROp_Header>(Arg)->Op == OP_INLINEENTRYPOINTOFFSET) {
            continue;
          }
          if (IR->GetOp<IROp_Header>(Arg)->Op == OP_IRHEADER) {
            continue;
          }

          const auto ArgNode = Arg.ID();
          auto& ArgNodeLiveRange = LiveRanges[ArgNode.Value];
          LOGMAN_THROW_AA_FMT(ArgNodeLiveRange.Begin.Value != UINT32_MAX,
                             "%{} used by %{} before defined?", ArgNode, Node);

          const auto ArgNodeBlockID = Graph->Nodes[ArgNode.Value].Head.BlockID;
          if (ArgNodeBlockID == BlockNodeID) {
            // Set the node end to be at least here
            ArgNodeLiveRange.End = Node;
          } else {
            ArgNodeLiveRange.Global = true;

            // Grow the live range to include this use
            ArgNodeLiveRange.Begin = std::min(ArgNodeLiveRange.Begin, Node);
            ArgNodeLiveRange.End = std::max(ArgNodeLiveRange.End, Node);

            // Can't spill this range, it is MB
            ArgNodeLiveRange.RematCost = -1;

            // Include any blocks this value passes through in the live range
            RecursiveLiveRangeExpansion(IR, ArgNode, ArgNodeBlockID, &ArgNodeLiveRange,
                                        Graph->BlockPredecessors[BlockNodeID],
                                        Graph->VisitedNodePredecessors[ArgNode]);
          }
        }
      }
    }
  }

  void ConstrainedRAPass::OptimizeStaticRegisters(FEXCore::IR::IRListView *IR) {

    // Helpers

    // Is an OP_STOREREGISTER eligible to write directly to the SRA reg?
    auto IsPreWritable = [this](uint8_t Size, RegisterClassType StaticClass) {
      LOGMAN_THROW_A_FMT(StaticClass == GPRFixedClass || StaticClass == FPRFixedClass, "Unexpected static class {}", StaticClass);
      if (StaticClass == GPRFixedClass) {
        return Size == 8 || Size == 4;
      } else if (StaticClass == FPRFixedClass) {
        return Size == 16 || (Size == 32 && SupportsAVX);
      }
      return false; // Unknown
    };

    // Is an OP_LOADREGISTER eligible to read directly from the SRA reg?
    auto IsAliasable = [this](uint8_t Size, RegisterClassType StaticClass, uint32_t Offset) {
      LOGMAN_THROW_A_FMT(StaticClass == GPRFixedClass || StaticClass == FPRFixedClass, "Unexpected static class {}", StaticClass);
      if (StaticClass == GPRFixedClass) {
        // We need more meta info to support not-size-of-reg
        return (Size == 8 || Size == 4) && ((Offset & 7) == 0);
      } else if (StaticClass == FPRFixedClass) {
        // We need more meta info to support not-size-of-reg
        if (Size == 32 && SupportsAVX && (Offset & 31) == 0) {
          return true;
        }
        return (Size == 16 /*|| Size == 8 || Size == 4*/) && ((Offset & 15) == 0);
      }
      return false; // Unknown
    };

    const auto GetFPRBeginAndEnd = [this]() -> std::pair<ptrdiff_t, ptrdiff_t> {
      if (SupportsAVX) {
        return {
          offsetof(FEXCore::Core::CpuStateFrame, State.xmm.avx.data[0][0]),
          offsetof(FEXCore::Core::CpuStateFrame, State.xmm.avx.data[16][0]),
        };
      } else {
        return {
          offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[0][0]),
          offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[16][0]),
        };
      }
    };

    // Get SRA Reg and Class from a Context offset
    const auto GetRegAndClassFromOffset = [&, this](uint32_t Offset) {
        const auto beginGpr = offsetof(FEXCore::Core::CpuStateFrame, State.gregs[0]);
        const auto endGpr = offsetof(FEXCore::Core::CpuStateFrame, State.gregs[16]);

        const auto [beginFpr, endFpr] = GetFPRBeginAndEnd();

        LOGMAN_THROW_AA_FMT((Offset >= beginGpr && Offset < endGpr) || (Offset >= beginFpr && Offset < endFpr), "Unexpected Offset {}", Offset);

        if (Offset >= beginGpr && Offset < endGpr) {
          auto reg = (Offset - beginGpr) / Core::CPUState::GPR_REG_SIZE;
          return PhysicalRegister(GPRFixedClass, reg);
        } else if (Offset >= beginFpr && Offset < endFpr) {
          const auto size = SupportsAVX ? Core::CPUState::XMM_AVX_REG_SIZE
                                        : Core::CPUState::XMM_SSE_REG_SIZE;
          const auto reg = (Offset - beginFpr) / size;
          return PhysicalRegister(FPRFixedClass, reg);
        }

        return PhysicalRegister::Invalid();
    };

    auto GprSize = Graph->Set.Classes[GPRFixedClass.Val].PhysicalCount;
    auto MapsSize =  Graph->Set.Classes[GPRFixedClass.Val].PhysicalCount + Graph->Set.Classes[FPRFixedClass.Val].PhysicalCount;
    LiveRange* StaticMaps[MapsSize];

    // Get a StaticMap entry from context offset
    const auto GetStaticMapFromOffset = [&](uint32_t Offset) -> LiveRange** {
        const auto beginGpr = offsetof(FEXCore::Core::CpuStateFrame, State.gregs[0]);
        const auto endGpr = offsetof(FEXCore::Core::CpuStateFrame, State.gregs[16]);

        const auto [beginFpr, endFpr] = GetFPRBeginAndEnd();

        LOGMAN_THROW_AA_FMT((Offset >= beginGpr && Offset < endGpr) || (Offset >= beginFpr && Offset < endFpr), "Unexpected Offset {}", Offset);

        if (Offset >= beginGpr && Offset < endGpr) {
          auto reg = (Offset - beginGpr) / Core::CPUState::GPR_REG_SIZE;
          return &StaticMaps[reg];
        } else if (Offset >= beginFpr && Offset < endFpr) {
          const auto size = SupportsAVX ? Core::CPUState::XMM_AVX_REG_SIZE
                                        : Core::CPUState::XMM_SSE_REG_SIZE;
          const auto reg = (Offset - beginFpr) / size;
          return &StaticMaps[GprSize + reg];
        }

        return nullptr;
    };

    // Get a StaticMap entry from reg and class
    const auto GetStaticMapFromReg = [&](IR::PhysicalRegister PhyReg) -> LiveRange** {
      LOGMAN_THROW_A_FMT(PhyReg.Class == GPRFixedClass.Val || PhyReg.Class == FPRFixedClass.Val, "Unexpected Class {}", PhyReg.Class);

      if (PhyReg.Class == GPRFixedClass.Val) {
        return &StaticMaps[PhyReg.Reg];
      } else if (PhyReg.Class == FPRFixedClass.Val) {
        return &StaticMaps[GprSize + PhyReg.Reg];
      }

      return nullptr;
    };

    // First pass: Mark pre-writes
    for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
      for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
        const auto Node = IR->GetID(CodeNode);

        if (IROp->Op == OP_STOREREGISTER) {
          auto Op = IROp->C<IR::IROp_StoreRegister>();
          const auto OpID = Op->Value.ID();
          auto& OpLiveRange = LiveRanges[OpID.Value];

          if (IsPreWritable(IROp->Size, Op->StaticClass)
            && OpLiveRange.PrefferedRegister.IsInvalid()
            && !OpLiveRange.Global) {

            // Pre-write and sra-allocate in the defining node - this might be undone if a read before the actual store happens
            SRA_DEBUG("Prewritting ssa{} (Store in ssa{})\n", OpID, Node);
            OpLiveRange.PrefferedRegister = GetRegAndClassFromOffset(Op->Offset);
            OpLiveRange.PreWritten = Node;
            SetNodeClass(Graph, OpID, Op->StaticClass);
          }
        }
      }
    }

    // Second pass:
    // - Demote pre-writes if read after pre-write
    // - Mark read-aliases
    // - Demote read-aliases if SRA reg is written before the alias's last read
    for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
      memset(StaticMaps, 0, MapsSize * sizeof(LiveRange*));
      for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
        const auto Node = IR->GetID(CodeNode);
        auto& NodeLiveRange = LiveRanges[Node.Value];

        // Check for read-after-write and demote if it happens
        const uint8_t NumArgs = IR::GetRAArgs(IROp->Op);
        for (uint8_t i = 0; i < NumArgs; ++i) {
          const auto& Arg = IROp->Args[i];

          if (Arg.IsInvalid()) {
            continue;
          }
          if (IR->GetOp<IROp_Header>(Arg)->Op == OP_INLINECONSTANT) {
            continue;
          }
          if (IR->GetOp<IROp_Header>(Arg)->Op == OP_INLINEENTRYPOINTOFFSET) {
            continue;
          }
          if (IR->GetOp<IROp_Header>(Arg)->Op == OP_IRHEADER) {
            continue;
          }

          const auto ArgNode = Arg.ID();
          auto& ArgNodeLiveRange = LiveRanges[ArgNode.Value];

          // ACCESSED after write, let's not SRA this one
          if (ArgNodeLiveRange.Written) {
            SRA_DEBUG("Demoting ssa{} because accessed after write in ssa{}\n", ArgNode, Node);
            ArgNodeLiveRange.PrefferedRegister = PhysicalRegister::Invalid();
            auto ArgNodeNode = IR->GetNode(Arg);
            SetNodeClass(Graph, ArgNode, GetRegClassFromNode(IR, ArgNodeNode->Op(IR->GetData())));
          }
        }

        // This op defines a span
        if (GetHasDest(IROp->Op)) {
          // If this is a pre-write, update the StaticMap so we track writes
          if (!NodeLiveRange.PrefferedRegister.IsInvalid()) {
            SRA_DEBUG("ssa{} is a pre-write\n", Node);
            auto StaticMap = GetStaticMapFromReg(NodeLiveRange.PrefferedRegister);
            if ((*StaticMap)) {
              SRA_DEBUG("Markng ssa{} as written because ssa{} writes to sra{}\n",
                        (*StaticMap) - &LiveRanges[0], Node, -1 /*vreg*/);
              (*StaticMap)->Written = true;
            }
            (*StaticMap) = &NodeLiveRange;
          }

          // Opcode is an SRA read
          // Check if
          // - There is not a pre-write before this read. If there is one, demote to no pre-write
          // - Try to read-alias if possible
          if (IROp->Op == OP_LOADREGISTER) {
            auto Op = IROp->C<IR::IROp_LoadRegister>();

            auto StaticMap = GetStaticMapFromOffset(Op->Offset);

            // Make sure there wasn't a store pre-written before this read
            if ((*StaticMap) && (*StaticMap)->PreWritten.IsValid()) {
              const auto ID = IR::NodeID((*StaticMap) - &LiveRanges[0]);

              SRA_DEBUG("ssa{} cannot be a pre-write because ssa{} reads from sra{} before storereg",
                        ID, Node, -1 /*vreg*/);
              (*StaticMap)->PrefferedRegister = PhysicalRegister::Invalid();
              (*StaticMap)->PreWritten.Invalidate();
              SetNodeClass(Graph, ID, Op->Class);
            }

            // if not sra-allocated and full size, sra-allocate
            if (!NodeLiveRange.Global && NodeLiveRange.PrefferedRegister.IsInvalid()) {
              // only full size reads can be aliased
              if (IsAliasable(IROp->Size, Op->StaticClass, Op->Offset)) {
                // We can only track a single active span.
                // Marking here as written is overly agressive, but
                // there might be write(s) later on the instruction stream
                if ((*StaticMap)) {
                  SRA_DEBUG("Markng ssa{} as written because ssa{} re-loads sra{}, and we can't track possible future writes\n",
                            (*StaticMap) - &LiveRanges[0], Node, -1 /*vreg*/);
                  (*StaticMap)->Written = true;
                }

                NodeLiveRange.PrefferedRegister = GetRegAndClassFromOffset(Op->Offset); //0, 1, and so on
                (*StaticMap) = &NodeLiveRange;
                SetNodeClass(Graph, Node, Op->StaticClass);
                SRA_DEBUG("Marking ssa{} as allocated to sra{}\n", Node, -1 /*vreg*/);
              }
            }
          }
        }

        // OP is an OP_STOREREGISTER
        // - If there was a matching pre-write, clear the pre-write flag as the register is no longer pre-written
        // - Mark the SRA span as written, so that any further reads demote it from read-aliases if they happen
        if (IROp->Op == OP_STOREREGISTER) {
          const auto Op = IROp->C<IR::IROp_StoreRegister>();
          const auto OpID = Op->Value.ID();
          auto& OpLiveRange = LiveRanges[OpID.Value];

          auto StaticMap = GetStaticMapFromOffset(Op->Offset);
          // if a read pending, it has been writting
          if ((*StaticMap)) {
            // writes to self don't invalidate the span
            if ((*StaticMap)->PreWritten != Node) {
              SRA_DEBUG("Marking ssa{} as written because ssa{} writes to sra{} with value ssa{}. Write size is {}\n",
                        ID, Node, -1 /*vreg*/, OpID, IROp->Size);
              (*StaticMap)->Written = true;
            }
          }
          if (OpLiveRange.PreWritten == Node) {
            // no longer pre-written
            OpLiveRange.PreWritten.Invalidate();
            SRA_DEBUG("Marking ssa{} as no longer pre-written as ssa{} is a storereg for sra{}\n",
                      OpID, Node, -1 /*vreg*/);
          }
        }
      }
    }
  }

  void ConstrainedRAPass::CalculateBlockInterferences(FEXCore::IR::IRListView *IR) {
    using namespace FEXCore;

    for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
      auto BlockIROp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();
      LOGMAN_THROW_AA_FMT(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

      const auto BlockNodeID = IR->GetID(BlockNode);
      const auto BlockBeginID = BlockIROp->Begin.ID();
      const auto BlockLastID = BlockIROp->Last.ID();

      auto& BlockInterferenceVector = LocalBlockInterferences.try_emplace(BlockNodeID).first->second;
      BlockInterferenceVector.reserve(BlockLastID.Value - BlockBeginID.Value);

      for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
        const auto Node = IR->GetID(CodeNode);
        LiveRange& NodeLiveRange = LiveRanges[Node.Value];

        if (NodeLiveRange.Begin >= BlockBeginID &&
            NodeLiveRange.End <= BlockLastID) {
          // If the live range of this node is FULLY inside of the block
          // Then add it to the block specific interference list
          BlockInterferenceVector.emplace_back(Node);
        }
        else {
          // If the live range is not fully inside the block then add it to the global interference list
          GlobalBlockInterferences.emplace_back(Node);
        }
      }
    }
  }

  void ConstrainedRAPass::CalculateBlockNodeInterference(FEXCore::IR::IRListView *IR) {
    #if 0
    const auto AddInterference = [&](IR::NodeID Node1, IR::NodeID Node2) {
      RegisterNode *Node = &Graph->Nodes[Node1.Value];
      Node->Interference.Set(Node2);
      Node->InterferenceList[Node->Head.InterferenceCount++] = Node2;
    };

    const auto CheckInterferenceNodeSizes = [&](IR::NodeID Node1, uint32_t MaxNewNodes) {
      RegisterNode *Node = &Graph->Nodes[Node1.Value];
      uint32_t NewListMax = Node->Head.InterferenceCount + MaxNewNodes;
      if (Node->InterferenceListSize <= NewListMax) {
        const auto AlignedListCount = static_cast<uint32_t>(FEXCore::AlignUp(NewListMax, DEFAULT_INTERFERENCE_LIST_COUNT));
        Node->InterferenceListSize = std::max(Node->InterferenceListSize * 2U, AlignedListCount);
        Node->InterferenceList = reinterpret_cast<uint32_t*>(realloc(Node->InterferenceList, Node->InterferenceListSize * sizeof(uint32_t)));
      }
    };
    using namespace FEXCore;

    for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
      BlockInterferences *BlockInterferenceVector = &LocalBlockInterferences.try_emplace(IR->GetID(BlockNode)).first->second;

      fextl::vector<IR::NodeID> Interferences;
      Interferences.reserve(BlockInterferenceVector->size() + GlobalBlockInterferences.size());

      for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
        const auto Node = IR->GetID(CodeNode);
        const auto& NodeLiveRange = LiveRanges[Node.Value];

        // Check for every interference with the local block's interference
        for (auto RHSNode : *BlockInterferenceVector) {
          const auto& RHSNodeLiveRange = LiveRanges[RHSNode.Value];

          if (!(NodeLiveRange.Begin >= RHSNodeLiveRange.End ||
                RHSNodeLiveRange.Begin >= NodeLiveRange.End)) {
            Interferences.emplace_back(RHSNode);
          }
        }

        // Now check the global block interference vector
        for (auto RHSNode : GlobalBlockInterferences) {
          const auto& RHSNodeLiveRange = LiveRanges[RHSNode.Value];

          if (!(NodeLiveRange.Begin >= RHSNodeLiveRange.End ||
                RHSNodeLiveRange.Begin >= NodeLiveRange.End)) {
            Interferences.emplace_back(RHSNode);
          }
        }

        CheckInterferenceNodeSizes(Node, Interferences.size());
        for (auto RHSNode : Interferences) {
          AddInterference(Node, RHSNode);
        }

        for (auto RHSNode : Interferences) {
          AddInterference(RHSNode, Node);
          CheckInterferenceNodeSizes(RHSNode, 0);
        }

        Interferences.clear();
      }
    }
    #endif
  }

  void ConstrainedRAPass::CalculateNodeInterference(FEXCore::IR::IRListView *IR) {
    const auto AddInterference = [this](IR::NodeID Node1, IR::NodeID Node2) {
      RegisterNode *Node = &Graph->Nodes[Node1.Value];
      Node->Interferences.Append(Node2);
    };

    const uint32_t NodeCount = IR->GetSSACount();

    // Now that we have all the live ranges calculated we need to add them to our interference graph

    const auto GetClass = [](PhysicalRegister PhyReg) {
      if (PhyReg.Class == IR::GPRPairClass.Val)
        return IR::GPRClass.Val;
      else
        return (uint32_t)PhyReg.Class;
    };

    // SpanStart/SpanEnd assume SSA id will fit in 24bits
    LOGMAN_THROW_AA_FMT(NodeCount <= 0xff'ffff, "Block too large for Spans");

    SpanStart.resize(NodeCount);
    SpanEnd.resize(NodeCount);
    for (uint32_t i = 0; i < NodeCount; ++i) {
      const auto& NodeLiveRange = LiveRanges[i];

      if (NodeLiveRange.Begin.Value != UINT32_MAX) {
        LOGMAN_THROW_A_FMT(NodeLiveRange.Begin < NodeLiveRange.End , "Span must Begin before Ending");

        const auto Class = GetClass(Graph->AllocData->Map[i]);
        SpanStart[NodeLiveRange.Begin.Value].Append(InfoMake(i, Class));
        SpanEnd[NodeLiveRange.End.Value]    .Append(InfoMake(i, Class));
      }
    }

    BucketList<32, uint32_t> Active;
    for (size_t OpNodeId = 0; OpNodeId < IR->GetSSACount(); OpNodeId++) {
      // Expire end intervals first
      SpanEnd[OpNodeId].Iterate([&](uint32_t EdgeInfo) {
        Active.Erase(InfoIDClass(EdgeInfo));
      });

      // Add starting invervals
      SpanStart[OpNodeId].Iterate([&](uint32_t EdgeInfo) {
        // Starts here
        Active.Iterate([&](uint32_t ActiveInfo) {
          if (InfoClass(ActiveInfo) == InfoClass(EdgeInfo)) {
            AddInterference(InfoID(ActiveInfo), InfoID(EdgeInfo));
            AddInterference(InfoID(EdgeInfo), InfoID(ActiveInfo));
          }
        });
        Active.Append(EdgeInfo);
      });
    }

    LOGMAN_THROW_AA_FMT(Active.Items[0] == 0, "Interference bug");
    SpanStart.clear();
    SpanEnd.clear();
  }

  void ConstrainedRAPass::AllocateVirtualRegisters() {
    for (uint32_t i = 0; i < Graph->NodeCount; ++i) {
      RegisterNode *CurrentNode = &Graph->Nodes[i];
      auto &CurrentRegAndClass = Graph->AllocData->Map[i];
      if (CurrentRegAndClass == PhysicalRegister::Invalid())
        continue;

      auto LiveRange = &LiveRanges[i];

      FEXCore::IR::RegisterClassType RegClass = FEXCore::IR::RegisterClassType{CurrentRegAndClass.Class};
      auto RegAndClass = PhysicalRegister::Invalid();
      RegisterClass *RAClass = &Graph->Set.Classes[RegClass];

      if (!LiveRange->PrefferedRegister.IsInvalid()) {
        RegAndClass = LiveRange->PrefferedRegister;
      } else {
        uint32_t RegisterConflicts = 0;
        CurrentNode->Interferences.Iterate([&](const IR::NodeID InterferenceNode) {
          RegisterConflicts |= GetConflicts(Graph, Graph->AllocData->Map[InterferenceNode.Value], {RegClass});
        });

        RegisterConflicts = (~RegisterConflicts) & RAClass->CountMask;

        int Reg = FindFirstSetBit(RegisterConflicts);
        if (Reg != 0) {
          RegAndClass = PhysicalRegister({RegClass}, Reg-1);
        }
      }

      // If we failed to find a virtual register then use INVALID_REG and mark allocation as failed
      if (RegAndClass.IsInvalid()) {
        RegAndClass = IR::PhysicalRegister(RegClass, INVALID_REG);
        HadFullRA = false;
        SpillPointId = IR::NodeID{i};

        CurrentRegAndClass = RegAndClass;
        // Must spill and restart
        return;
      }

      CurrentRegAndClass = RegAndClass;
    }
  }

  FEXCore::IR::AllNodesIterator ConstrainedRAPass::FindFirstUse(FEXCore::IR::IREmitter *IREmit, FEXCore::IR::OrderedNode* Node, FEXCore::IR::AllNodesIterator Begin, FEXCore::IR::AllNodesIterator End) {
    using namespace FEXCore::IR;
    const auto SearchID = IREmit->ViewIR().GetID(Node);

    while(1) {
      auto [RealNode, IROp] = Begin();

      const uint8_t NumArgs = FEXCore::IR::GetRAArgs(IROp->Op);
      for (uint8_t i = 0; i < NumArgs; ++i) {
        const auto ArgNode = IROp->Args[i].ID();
        if (ArgNode == SearchID) {
          return Begin;
        }
      }

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (Begin == End) {
        break;
      }

      ++Begin;
    }

    return AllNodesIterator::Invalid();
  }

  FEXCore::IR::AllNodesIterator ConstrainedRAPass::FindLastUseBefore(FEXCore::IR::IREmitter *IREmit, FEXCore::IR::OrderedNode* Node, FEXCore::IR::AllNodesIterator Begin, FEXCore::IR::AllNodesIterator End) {
    auto CurrentIR = IREmit->ViewIR();
    const auto SearchID = CurrentIR.GetID(Node);

    while (1) {
      using namespace FEXCore::IR;
      auto [RealNode, IROp] = End();

      if (Node == RealNode) {
        // We walked back all the way to the definition of the IR op
        return End;
      }

      const uint8_t NumArgs = FEXCore::IR::GetRAArgs(IROp->Op);
      for (uint8_t i = 0; i < NumArgs; ++i) {
        const auto ArgNode = IROp->Args[i].ID();
        if (ArgNode == SearchID) {
          return End;
        }
      }

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (Begin == End) {
        break;
      }

      --End;
    }

    return FEXCore::IR::AllNodesIterator::Invalid();
  }

  std::optional<IR::NodeID> ConstrainedRAPass::FindNodeToSpill(IREmitter *IREmit,
                                                               RegisterNode *RegisterNode,
                                                               IR::NodeID CurrentLocation,
                                                               LiveRange const *OpLiveRange,
                                                               int32_t RematCost) {
    auto IR = IREmit->ViewIR();

    IR::NodeID InterferenceIdToSpill{};
    uint32_t InterferenceFarthestNextUse = 0;

    IR::OrderedNodeWrapper NodeOpBegin = IR::OrderedNodeWrapper::WrapOffset(CurrentLocation.Value * sizeof(IR::OrderedNode));
    IR::OrderedNodeWrapper NodeOpEnd = IR::OrderedNodeWrapper::WrapOffset(OpLiveRange->End.Value * sizeof(IR::OrderedNode));
    auto NodeOpBeginIter = IR.at(NodeOpBegin);
    auto NodeOpEndIter = IR.at(NodeOpEnd);

    // Couldn't find register to spill
    // Be more aggressive
    if (InterferenceIdToSpill.IsInvalid()) {
      RegisterNode->Interferences.Iterate([&](IR::NodeID InterferenceNode) {
        auto *InterferenceLiveRange = &LiveRanges[InterferenceNode.Value];
        if (InterferenceLiveRange->RematCost == -1 ||
            (RematCost != -1 && InterferenceLiveRange->RematCost != RematCost)) {
          return;
        }

        //if ((RegisterNode->Head.RegAndClass>>32) != (InterferenceNode->Head.RegAndClass>>32))
        //  return;

        // If this node's live range fully encompasses the live range of the interference node
        // then spilling that interference node will not lower RA
        // | Our Node             |        Interference |
        // | ========================================== |
        // | 0 - Assign           |                     |
        // | 1                    |              Assign |
        // | 2                    |                     |
        // | 3                    |            Last Use |
        // | 4                    |                     |
        // | 5 - Last Use         |                     |
        // | Range - (0, 5]       |              (1, 3] |
        if (OpLiveRange->Begin <= InterferenceLiveRange->Begin &&
            OpLiveRange->End >= InterferenceLiveRange->End) {
          return;
        }

        auto [InterferenceOrderedNode, _] = IR.at(InterferenceNode)();
        auto InterferenceNodeOpBeginIter = IR.at(InterferenceLiveRange->Begin);
        auto InterferenceNodeOpEndIter = IR.at(InterferenceLiveRange->End);

        // If the nodes live range is entirely encompassed by the interference node's range
        // then spilling that range will /potentially/ lower RA
        // Will only lower register pressure if the interference node does NOT have a use inside of
        // this live range's use
        // | Our Node             |        Interference |
        // | ========================================== |
        // | 0                    |              Assign |
        // | 1 - Assign           |            (No Use) |
        // | 2                    |            (No Use) |
        // | 3 - Last Use         |            (No Use) |
        // | 4                    |                     |
        // | 5                    |            Last Use |
        // | Range - (1, 3]       |              (0, 5] |
        if (CurrentLocation > InterferenceLiveRange->Begin &&
            OpLiveRange->End < InterferenceLiveRange->End) {

          // This will only save register pressure if the interference node
          // does NOT have a use inside of this this node's live range
          // Search only inside the source node's live range to see if there is a use
          auto FirstUseLocation = FindFirstUse(IREmit, InterferenceOrderedNode, NodeOpBeginIter, NodeOpEndIter);
          if (FirstUseLocation == IR::NodeIterator::Invalid()) {
            // Looks like there isn't a usage of this interference node inside our node's live range
            // This means it is safe to spill this node and it'll result in in lower RA
            // Proper calculation of cost to spill would be to calculate the two distances from
            // (Node->Begin - InterferencePrevUse) + (InterferenceNextUse - Node->End)
            // This would ensure something will spill earlier if its previous use and next use are farther away
            auto InterferenceNodeNextUse = FindFirstUse(IREmit, InterferenceOrderedNode, NodeOpBeginIter, InterferenceNodeOpEndIter);
            auto InterferenceNodePrevUse = FindLastUseBefore(IREmit, InterferenceOrderedNode, InterferenceNodeOpBeginIter, NodeOpBeginIter);
            LOGMAN_THROW_A_FMT(InterferenceNodeNextUse != IR::NodeIterator::Invalid(), "Couldn't find next usage of op");
            // If there is no use of the interference op prior to our op then it only has initial definition
            if (InterferenceNodePrevUse == IR::NodeIterator::Invalid()) {
              InterferenceNodePrevUse = InterferenceNodeOpBeginIter;
            }

            const auto NextUseDistance = InterferenceNodeNextUse.ID().Value - CurrentLocation.Value;
            if (NextUseDistance >= InterferenceFarthestNextUse) {
              InterferenceIdToSpill = InterferenceNode;
              InterferenceFarthestNextUse = NextUseDistance;
            }
          }
        }
      });
    }


    if (InterferenceIdToSpill.IsInvalid()) {
      RegisterNode->Interferences.Iterate([&](IR::NodeID InterferenceNode) {
        auto *InterferenceLiveRange = &LiveRanges[InterferenceNode.Value];
        if (InterferenceLiveRange->RematCost == -1 ||
            (RematCost != -1 && InterferenceLiveRange->RematCost != RematCost)) {
          return;
        }

        // If this node's live range fully encompasses the live range of the interference node
        // then spilling that interference node will not lower RA
        // | Our Node             |        Interference |
        // | ========================================== |
        // | 0 - Assign           |                     |
        // | 1                    |              Assign |
        // | 2                    |                     |
        // | 3                    |            Last Use |
        // | 4                    |                     |
        // | 5 - Last Use         |                     |
        // | Range - (0, 5]       |              (1, 3] |
        if (OpLiveRange->Begin <= InterferenceLiveRange->Begin &&
            OpLiveRange->End >= InterferenceLiveRange->End) {
          return;
        }

        auto [InterferenceOrderedNode, _] = IR.at(InterferenceNode)();
        auto InterferenceNodeOpEndIter = IR.at(InterferenceLiveRange->End);

        bool Found{};

        // If the node's live range intersects the interference node
        // but the interference node only overlaps the beginning of our live range
        // then spilling the register will lower register pressure if there is not
        // a use of the interference register at the same node as assignment
        // (So we can spill just before current node assignment)
        // | Our Node             |        Interference |
        // | ========================================== |
        // | 0                    |              Assign |
        // | 1 - Assign           |            (No Use) |
        // | 2                    |            (No Use) |
        // | 3                    |            Last Use |
        // | 4                    |                     |
        // | 5 - Last Use         |                     |
        // | Range - (1, 5]       |              (0, 3] |
        if (!Found &&
            CurrentLocation > InterferenceLiveRange->Begin &&
            OpLiveRange->End > InterferenceLiveRange->End) {
          auto FirstUseLocation = FindFirstUse(IREmit, InterferenceOrderedNode, NodeOpBeginIter, NodeOpBeginIter);

          if (FirstUseLocation == IR::NodeIterator::Invalid()) {
            // This means that the assignment of our register doesn't use this interference node
            // So we are safe to spill this interference node before assignment of our current node
            const auto InterferenceNodeNextUse = FindFirstUse(IREmit, InterferenceOrderedNode, NodeOpBeginIter, InterferenceNodeOpEndIter);
            const auto NextUseDistance = InterferenceNodeNextUse.ID().Value - CurrentLocation.Value;
            if (NextUseDistance >= InterferenceFarthestNextUse) {
              Found = true;

              InterferenceIdToSpill = InterferenceNode;
              InterferenceFarthestNextUse = NextUseDistance;
            }
          }
        }

        // If the node's live range intersects the interference node
        // but the interference node only overlaps the end of our live range
        // then spilling the register will lower register pressure if there is
        // not a use of the interference register at the same node as the other node's
        // last use
        // | Our Node             |        Interference |
        // | ========================================== |
        // | 0 - Assign           |                     |
        // | 1                    |                     |
        // | 2                    |              Assign |
        // | 3 - Last Use         |            (No Use) |
        // | 4                    |            (No Use) |
        // | 5                    |            Last Use |
        // | Range - (1, 3]       |              (2, 5] |

        // XXX: This route has a bug in it so it is purposely disabled for now
        if (false && !Found &&
            CurrentLocation <= InterferenceLiveRange->Begin &&
            OpLiveRange->End <= InterferenceLiveRange->End) {
          auto FirstUseLocation = FindFirstUse(IREmit, InterferenceOrderedNode, NodeOpEndIter, NodeOpEndIter);

          if (FirstUseLocation == IR::NodeIterator::Invalid()) {
            // This means that the assignment of our the interference register doesn't overlap
            // with the final usage of our register, we can spill it and reduce usage
            const auto InterferenceNodeNextUse = FindFirstUse(IREmit, InterferenceOrderedNode, NodeOpBeginIter, InterferenceNodeOpEndIter);
            const auto NextUseDistance = InterferenceNodeNextUse.ID().Value - CurrentLocation.Value;
            if (NextUseDistance >= InterferenceFarthestNextUse) {
              Found = true;

              InterferenceIdToSpill = InterferenceNode;
              InterferenceFarthestNextUse = NextUseDistance;
            }
          }
        }
      });
    }

    // If we are looking for a specific node then we can safely return not found
    if (RematCost != -1 && InterferenceIdToSpill.IsInvalid()) {
      return std::nullopt;
    }

    // Heuristics failed to spill ?
    if (InterferenceIdToSpill.IsInvalid()) {
      // Panic spill: Spill any value not used by the current op
      fextl::set<IR::NodeID> CurrentNodes;

      // Get all used nodes for current IR op
      {
          auto CurrentNode = IR.GetNode(NodeOpBegin);
          auto IROp = CurrentNode->Op(IR.GetData());

          CurrentNodes.insert(NodeOpBegin.ID());

          for (int i = 0; i < IR::GetRAArgs(IROp->Op); i++) {
            CurrentNodes.insert(IROp->Args[i].ID());
          }
      }


      RegisterNode->Interferences.Find([&](IR::NodeID InterferenceNode) {
          auto *InterferenceLiveRange = &LiveRanges[InterferenceNode.Value];
          if (InterferenceLiveRange->RematCost == -1 ||
              (RematCost != -1 && InterferenceLiveRange->RematCost != RematCost)) {
            return false;
          }

        if (!CurrentNodes.contains(InterferenceNode)) {
          InterferenceIdToSpill = InterferenceNode;
          LogMan::Msg::DFmt("Panic spilling %{}, Live Range[{}, {})", InterferenceIdToSpill, InterferenceLiveRange->Begin, InterferenceLiveRange->End);
          return true;
        }
        return false;
      });
    }

    if (InterferenceIdToSpill.IsInvalid()) {
      int j = 0;
      LogMan::Msg::DFmt("node %{}, was dumped in to virtual reg {}. Live Range[{}, {})",
                        CurrentLocation, -1,
                        OpLiveRange->Begin, OpLiveRange->End);

      RegisterNode->Interferences.Iterate([&](IR::NodeID InterferenceNode) {
        auto *InterferenceLiveRange = &LiveRanges[InterferenceNode.Value];

        LogMan::Msg::DFmt("\tInt{}: %{} Remat: {} [{}, {})", j++, InterferenceNode, InterferenceLiveRange->RematCost, InterferenceLiveRange->Begin, InterferenceLiveRange->End);
      });
    }
    LOGMAN_THROW_A_FMT(InterferenceIdToSpill.IsValid(), "Couldn't find Node to spill");

    return InterferenceIdToSpill;
  }

  uint32_t ConstrainedRAPass::FindSpillSlot(IR::NodeID Node, FEXCore::IR::RegisterClassType RegisterClass) {
    RegisterNode& CurrentNode = Graph->Nodes[Node.Value];
    const auto& NodeLiveRange = LiveRanges[Node.Value];

    if (ReuseSpillSlots) {
      for (uint32_t i = 0; i < Graph->SpillStack.size(); ++i) {
        SpillStackUnit& SpillUnit = Graph->SpillStack[i];

        if (NodeLiveRange.Begin <= SpillUnit.SpillRange.End &&
            SpillUnit.SpillRange.Begin <= NodeLiveRange.End) {
          SpillUnit.SpillRange.Begin = std::min(SpillUnit.SpillRange.Begin, NodeLiveRange.Begin);
          SpillUnit.SpillRange.End = std::max(SpillUnit.SpillRange.End, NodeLiveRange.End);
          CurrentNode.Head.SpillSlot = i;
          return i;
        }
      }
    }

    // Couldn't find a spill slot so just make a new one
    auto StackItem = Graph->SpillStack.emplace_back(SpillStackUnit{Node, RegisterClass});
    StackItem.SpillRange.Begin = NodeLiveRange.Begin;
    StackItem.SpillRange.End = NodeLiveRange.End;
    CurrentNode.Head.SpillSlot = SpillSlotCount;
    SpillSlotCount++;
    return CurrentNode.Head.SpillSlot;
  }

  void ConstrainedRAPass::SpillOne(FEXCore::IR::IREmitter *IREmit) {
    using namespace FEXCore;

    auto IR = IREmit->ViewIR();
    auto LastCursor = IREmit->GetWriteCursor();
    auto [CodeNode, IROp] = IR.at(SpillPointId)();

    LOGMAN_THROW_AA_FMT(GetHasDest(IROp->Op), "Can't spill with no dest");

    const auto Node = IR.GetID(CodeNode);
    RegisterNode *CurrentNode = &Graph->Nodes[Node.Value];
    auto &CurrentRegAndClass = Graph->AllocData->Map[Node.Value];
    LiveRange *OpLiveRange = &LiveRanges[Node.Value];

    // If this node is allocated above the number of physical registers
    // we have then we need to search the interference list and spill the one
    // that is cheapest
    const bool NeedsToSpill = CurrentRegAndClass.Reg == INVALID_REG;

    if (NeedsToSpill) {
      bool Spilled = false;

      // First let's just check for constants that we can just rematerialize instead of spilling
      if (const auto InterferenceNode = FindNodeToSpill(IREmit, CurrentNode, Node, OpLiveRange, 1)) {
        // We want to end the live range of this value here and continue it on first use
        auto [ConstantNode, _] = IR.at(*InterferenceNode)();
        auto ConstantIROp = IR.GetOp<IR::IROp_Constant>(ConstantNode);

        // First op post Spill
        auto NextIter = IR.at(CodeNode);
        auto FirstUseLocation = FindFirstUse(IREmit, ConstantNode, NextIter, NodeIterator::Invalid());

        LOGMAN_THROW_A_FMT(FirstUseLocation != IR::NodeIterator::Invalid(),
                           "At %{} Spilling Op %{} but Failure to find op use",
                           Node, *InterferenceNode);

        if (FirstUseLocation != IR::NodeIterator::Invalid()) {
          --FirstUseLocation;
          auto [FirstUseOrderedNode, _] = FirstUseLocation();
          IREmit->SetWriteCursor(FirstUseOrderedNode);
          auto FilledConstant = IREmit->_Constant(ConstantIROp->Constant);
          IREmit->ReplaceUsesWithAfter(ConstantNode, FilledConstant, FirstUseLocation);
          Spilled = true;
        }
      }

      // If we didn't remat a constant then we need to do some real spilling
      if (!Spilled) {
        if (const auto InterferenceNode = FindNodeToSpill(IREmit, CurrentNode, Node, OpLiveRange)) {
          const auto InterferenceRegClass = IR::RegisterClassType{Graph->AllocData->Map[InterferenceNode->Value].Class};
          const uint32_t SpillSlot = FindSpillSlot(*InterferenceNode, InterferenceRegClass);

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
          LOGMAN_THROW_A_FMT(SpillSlot != UINT32_MAX, "Interference Node doesn't have a spill slot!");
          LOGMAN_THROW_A_FMT(InterferenceRegClass != UINT32_MAX, "Interference node never assigned a register class?");
#endif

          // This is the op that we need to dump
          auto [InterferenceOrderedNode, InterferenceIROp] = IR.at(*InterferenceNode)();


          // This will find the last use of this definition
          // Walks from CodeBegin -> BlockBegin to find the last Use
          // Which this is walking backwards to find the first use
          auto LastUseIterator = FindLastUseBefore(IREmit, InterferenceOrderedNode, NodeIterator::Invalid(), IR.at(CodeNode));
          if (LastUseIterator != AllNodesIterator::Invalid()) {
            auto [LastUseNode, LastUseIROp] = LastUseIterator();

            // Set the write cursor to point of last usage
            IREmit->SetWriteCursor(LastUseNode);
          } else {
            // There is no last use -- use the definition as last use
            IREmit->SetWriteCursor(InterferenceOrderedNode);
          }

          // Actually spill the node now
          auto SpillOp = IREmit->_SpillRegister(InterferenceOrderedNode, SpillSlot, InterferenceRegClass);
          SpillOp.first->Header.Size = InterferenceIROp->Size;
          SpillOp.first->Header.ElementSize = InterferenceIROp->ElementSize;

          {
            // Search from the point of spilling to find the first use
            // Set the write cursor to the first location found and fill at that point
            auto FirstIter = IR.at(SpillOp.Node);
            // Just past the spill
            ++FirstIter;
            auto FirstUseLocation = FindFirstUse(IREmit, InterferenceOrderedNode, FirstIter, NodeIterator::Invalid());

            LOGMAN_THROW_A_FMT(FirstUseLocation != NodeIterator::Invalid(),
                               "At %{} Spilling Op %{} but Failure to find op use",
                               Node, *InterferenceNode);

            if (FirstUseLocation != IR::NodeIterator::Invalid()) {
              // We want to fill just before the first use
              --FirstUseLocation;
              auto [FirstUseOrderedNode, _] = FirstUseLocation();

              IREmit->SetWriteCursor(FirstUseOrderedNode);

              auto FilledInterference = IREmit->_FillRegister(InterferenceOrderedNode, SpillSlot, InterferenceRegClass);
              FilledInterference.first->Header.Size = InterferenceIROp->Size;
              FilledInterference.first->Header.ElementSize = InterferenceIROp->ElementSize;
              IREmit->ReplaceUsesWithAfter(InterferenceOrderedNode, FilledInterference, FilledInterference);
              Spilled = true;
            }
          }
        }
        IREmit->SetWriteCursor(LastCursor);
      }
    }
  }

  bool ConstrainedRAPass::RunAllocateVirtualRegisters(FEXCore::IR::IREmitter *IREmit) {
    using namespace FEXCore;
    bool Changed = false;

    GlobalBlockInterferences.clear();
    LocalBlockInterferences.clear();

    auto IR = IREmit->ViewIR();

    uint32_t SSACount = IR.GetSSACount();

    ResetRegisterGraph(Graph, SSACount);
    FindNodeClasses(Graph, &IR);
    CalculateLiveRange(&IR);
    if (OptimizeSRA)
      OptimizeStaticRegisters(&IR);

    // Linear forward scan based interference calculation is faster for smaller blocks
    // Smarter block based interference calculation is faster for larger blocks
    /*if (SSACount >= 2048) {
      CalculateBlockInterferences(&IR);
      CalculateBlockNodeInterference(&IR);
    }
    else*/ {
      CalculateNodeInterference(&IR);
    }
    AllocateVirtualRegisters();

    return Changed;
  }


  void ConstrainedRAPass::CalculatePredecessors(FEXCore::IR::IRListView *IR) {
    Graph->BlockPredecessors.clear();

    for (auto [BlockNode, BlockIROp] : IR->GetBlocks()) {
      auto CodeBlock = BlockIROp->C<IROp_CodeBlock>();

      auto IROp = IR->GetNode(IR->GetNode(CodeBlock->Last)->Header.Previous)->Op(IR->GetData());
      if (IROp->Op == OP_JUMP) {
        auto Op = IROp->C<IROp_Jump>();
        Graph->BlockPredecessors[Op->TargetBlock.ID()].insert(IR->GetID(BlockNode));
      } else if (IROp->Op == OP_CONDJUMP) {
        auto Op = IROp->C<IROp_CondJump>();
        Graph->BlockPredecessors[Op->TrueBlock.ID()].insert(IR->GetID(BlockNode));
        Graph->BlockPredecessors[Op->FalseBlock.ID()].insert(IR->GetID(BlockNode));
      }
    }
  }

  bool ConstrainedRAPass::Run(IREmitter *IREmit) {
    FEXCORE_PROFILE_SCOPED("PassManager::RA");
    bool Changed = false;

    auto IR = IREmit->ViewIR();

    SpillSlotCount = 0;
    Graph->SpillStack.clear();

    CalculatePredecessors(&IR);

    while (1) {
      HadFullRA = true;

      // Virtual allocation pass runs the compaction pass per run
      Changed |= RunAllocateVirtualRegisters(IREmit);

      if (HadFullRA) {
        break;
      }

      SpillOne(IREmit);
      Changed = true;
      // We need to rerun compaction after spilling
      CompactionPass->Run(IREmit);
    }

    Graph->AllocData->SpillSlotCount = Graph->SpillStack.size();

    return Changed;
  }

  fextl::unique_ptr<FEXCore::IR::RegisterAllocationPass> CreateRegisterAllocationPass(FEXCore::IR::Pass* CompactionPass, bool OptimizeSRA, bool SupportsAVX) {
    return fextl::make_unique<ConstrainedRAPass>(CompactionPass, OptimizeSRA, SupportsAVX);
  }
}
