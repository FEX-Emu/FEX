#include "Common/BitSet.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"
#include "Interface/IR/Passes.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <iterator>

namespace {
  constexpr uint32_t INVALID_REG = ~0U;
  constexpr uint64_t INVALID_REGCLASS = ~0ULL;
  constexpr uint32_t DEFAULT_INTERFERENCE_LIST_COUNT = 128;
  constexpr uint32_t DEFAULT_NODE_COUNT = 8192;
  constexpr uint32_t DEFAULT_VIRTUAL_REG_COUNT = 1024;

  struct Register {
    bool Virtual;
    uint64_t Index;
  };

  struct RegisterClass {
    uint32_t Count;
    uint32_t PhysicalCount;
    std::vector<uint64_t> Conflicts;
  };

  struct RegisterNode {
    struct VolatileHeader {
      uint64_t RegAndClass;
      uint32_t InterferenceCount;
      uint32_t BlockID;
      uint32_t SpillSlot;
      RegisterNode *PhiPartner;
    } Head;

    uint32_t InterferenceListSize;
    uint32_t *InterferenceList;
    BitSetView<uint64_t> Interference;
  };
  static_assert(std::is_pod<RegisterNode>::value, "We want this to be POD");

  constexpr RegisterNode::VolatileHeader DefaultNodeHeader = {
    .RegAndClass = INVALID_REGCLASS,
    .InterferenceCount = 0,
    .BlockID = ~0U,
    .SpillSlot = ~0U,
    .PhiPartner = nullptr,
  };

  struct RegisterSet {
    std::vector<RegisterClass> Classes;
    uint32_t ClassCount;
  };

  struct LiveRange {
    uint32_t Begin;
    uint32_t End;
    uint32_t RematCost;
  };

  struct SpillStackUnit {
    uint32_t Node;
    FEXCore::IR::RegisterClassType Class;
    LiveRange SpillRange;
    FEXCore::IR::OrderedNode *SpilledNode;
  };

  struct RegisterGraph {
    RegisterSet Set;
    RegisterNode *Nodes;
    BitSet<uint64_t> InterferenceSet;
    uint32_t NodeCount;
    uint32_t MaxNodeCount;
    std::vector<SpillStackUnit> SpillStack;
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

  void AllocateRegisters(RegisterGraph *Graph, FEXCore::IR::RegisterClassType Class, uint32_t Count) {
    Graph->Set.Classes[Class].Count = Count;
  }

  void AllocatePhysicalRegisters(RegisterGraph *Graph, FEXCore::IR::RegisterClassType Class, uint32_t Count) {
    Graph->Set.Classes[Class].PhysicalCount = Count;
  }

  void VirtualAddRegisterConflict(RegisterGraph *Graph, FEXCore::IR::RegisterClassType ClassConflict, uint32_t RegConflict, FEXCore::IR::RegisterClassType Class, uint32_t Reg) {
    LogMan::Throw::A(Reg < Graph->Set.Classes[Class].Conflicts.size(), "Tried adding reg %d to conflict list only %d in size", RegConflict, Graph->Set.Classes[Class].Conflicts.size());
    LogMan::Throw::A(RegConflict < Graph->Set.Classes[ClassConflict].Conflicts.size(), "Tried adding reg %d to conflict list only %d in size", Reg, Graph->Set.Classes[ClassConflict].Conflicts.size());

    // Conflict must go both ways
    Graph->Set.Classes[Class].Conflicts[Reg] = {((uint64_t)ClassConflict << 32) | RegConflict};
    Graph->Set.Classes[ClassConflict].Conflicts[RegConflict] = {((uint64_t)Class << 32) | Reg};
  }

  void VirtualAllocateRegisterConflicts(RegisterGraph *Graph, FEXCore::IR::RegisterClassType Class, uint32_t NumConflicts) {
    Graph->Set.Classes[Class].Conflicts.resize(NumConflicts);
  }

  // Returns the new register ID that was the previous top
  uint32_t AllocateMoreRegisters(RegisterGraph *Graph, FEXCore::IR::RegisterClassType Class) {
    RegisterClass &LocalClass = Graph->Set.Classes[Class];
    uint32_t OldNumber = LocalClass.Count;
    LocalClass.Count *= 2;
    return OldNumber;
  }

  void FreeRegisterGraph(RegisterGraph *Graph) {
    for (size_t i = 0; i <Graph->MaxNodeCount; ++i) {
      free(Graph->Nodes[i].InterferenceList);
    }
    free(Graph->Nodes);
    Graph->InterferenceSet.Free();

    Graph->Set.Classes.clear();
    delete Graph;
  }

  void ResetRegisterGraph(RegisterGraph *Graph, uint64_t NodeCount) {
    NodeCount = AlignUp(NodeCount, sizeof(uint64_t));
    if (NodeCount > Graph->MaxNodeCount) {
      uint32_t OldNodeCount = Graph->MaxNodeCount;
      Graph->NodeCount = NodeCount;
      Graph->MaxNodeCount = NodeCount;
      Graph->Nodes = static_cast<RegisterNode*>(realloc(Graph->Nodes, NodeCount * sizeof(RegisterNode)));

      Graph->InterferenceSet.Realloc(NodeCount * NodeCount);
      Graph->InterferenceSet.MemClear(NodeCount * NodeCount);

      // Initialize nodes
      for (uint32_t i = 0; i < OldNodeCount; ++i) {
        Graph->Nodes[i].Head = DefaultNodeHeader;
        Graph->Nodes[i].Interference.GetView(Graph->InterferenceSet, NodeCount * i);
      }

      for (uint32_t i = OldNodeCount; i < NodeCount; ++i) {
        Graph->Nodes[i].Head = DefaultNodeHeader;
        Graph->Nodes[i].InterferenceListSize = DEFAULT_INTERFERENCE_LIST_COUNT;
        Graph->Nodes[i].InterferenceList = reinterpret_cast<uint32_t*>(calloc(Graph->Nodes[i].InterferenceListSize, sizeof(uint32_t)));
        Graph->Nodes[i].Interference.GetView(Graph->InterferenceSet, NodeCount * i);
      }
    }
    else {
      // We are only handling a node count of this size right now
      Graph->NodeCount = NodeCount;
      Graph->InterferenceSet.MemClear(NodeCount * NodeCount);

      // Initialize nodes
      for (uint32_t i = 0; i < NodeCount; ++i) {
        Graph->Nodes[i].Head = DefaultNodeHeader;
      }
    }
  }

  void SetNodeClass(RegisterGraph *Graph, uint32_t Node, FEXCore::IR::RegisterClassType Class) {
    Graph->Nodes[Node].Head.RegAndClass = ((uint64_t)Class << 32) | (Graph->Nodes[Node].Head.RegAndClass & ~0U);
  }

  void SetNodePartner(RegisterGraph *Graph, uint32_t Node, uint32_t Partner) {
    Graph->Nodes[Node].Head.PhiPartner = &Graph->Nodes[Partner];
  }

  /**
   * @brief Individual node interference check
   */
  bool DoesNodeInterfereWithRegister(RegisterGraph *Graph, RegisterNode const *Node, uint64_t RegAndClass) {
    // Walk the node's interference list and see if it interferes with this register
    for (uint32_t i = 0; i < Node->Head.InterferenceCount; ++i) {
      RegisterNode *InterferenceNode = &Graph->Nodes[Node->InterferenceList[i]];
      if (InterferenceNode->Head.RegAndClass == RegAndClass) {
        return true;
      }

      FEXCore::IR::RegisterClassType InterferenceClass = FEXCore::IR::RegisterClassType{(uint32_t)(InterferenceNode->Head.RegAndClass >> 32)};
      uint32_t InterferenceReg = InterferenceNode->Head.RegAndClass;
      if (InterferenceReg != INVALID_REG &&
          !Graph->Set.Classes[InterferenceClass].Conflicts.empty() &&
          Graph->Set.Classes[InterferenceClass].Conflicts[InterferenceReg] == RegAndClass) {
        return true;
      }
    }
    return false;
  }

  /**
   * @brief Node set walking for PHI node interference checking
   */
  bool DoesNodeSetInterfereWithRegister(RegisterGraph *Graph, std::vector<RegisterNode*> const &Nodes, uint64_t RegAndClass) {
    for (auto it : Nodes) {
      if (DoesNodeInterfereWithRegister(Graph, it, RegAndClass)) {
        return true;
      }
    }

    return false;
  }

  FEXCore::IR::RegisterClassType GetRegClassFromNode(FEXCore::IR::IRListView<false> *IR, FEXCore::IR::IROp_Header *IROp) {
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
      case IR::OP_PHIVALUE: {
        // Unwrap the PHIValue to get the class
        auto Op = IROp->C<IR::IROp_PhiValue>();
        return GetRegClassFromNode(IR, IR->GetOp<IR::IROp_Header>(Op->Value));
      }
      case IR::OP_PHI: {
        // Class is defined from the values passed in
        // All Phi nodes should have its class be the same (Validation should confirm this
        auto Op = IROp->C<IR::IROp_Phi>();
        return GetRegClassFromNode(IR, IR->GetOp<IR::IROp_Header>(Op->PhiBegin));
      }
      default: break;
    }

    // Unreachable
    return FEXCore::IR::InvalidClass;
  };

  // Walk the IR and set the node classes
  void FindNodeClasses(RegisterGraph *Graph, FEXCore::IR::IRListView<false> *IR) {
    for (auto [CodeNode, IROp] : IR->GetAllCode()) {
      // If the destination hasn't yet been set then set it now
      if (IROp->HasDest) {
        SetNodeClass(Graph, IR->GetID(CodeNode), GetRegClassFromNode(IR, IROp));
      }
    }
  }
}

namespace FEXCore::IR {
  class ConstrainedRAPass final : public RegisterAllocationPass {
    public:
      ConstrainedRAPass();
      ~ConstrainedRAPass();
      bool Run(IREmitter *IREmit) override;

      void AllocateRegisterSet(uint32_t RegisterCount, uint32_t ClassCount) override;
      void AddRegisters(FEXCore::IR::RegisterClassType Class, uint32_t RegisterCount) override;
      void AddRegisterConflict(FEXCore::IR::RegisterClassType ClassConflict, uint32_t RegConflict, FEXCore::IR::RegisterClassType Class, uint32_t Reg) override;
      void AllocateRegisterConflicts(FEXCore::IR::RegisterClassType Class, uint32_t NumConflicts) override;

      /**
       * @brief Returns the register and class encoded together
       * Top 32bits is the class, lower 32bits is the register
       */
      uint64_t GetNodeRegister(uint32_t Node) override;
    private:

      std::vector<uint32_t> PhysicalRegisterCount;
      std::vector<uint32_t> TopRAPressure;

      RegisterGraph *Graph;
      std::unique_ptr<FEXCore::IR::Pass> LocalCompaction;

      void SpillRegisters(FEXCore::IR::IREmitter *IREmit);

      std::vector<LiveRange> LiveRanges;

      using BlockInterferences = std::vector<uint32_t>;

      std::unordered_map<uint32_t, BlockInterferences> LocalBlockInterferences;
      BlockInterferences GlobalBlockInterferences;

      void CalculateLiveRange(FEXCore::IR::IRListView<false> *IR);
      void CalculateBlockInterferences(FEXCore::IR::IRListView<false> *IR);
      void CalculateBlockNodeInterference(FEXCore::IR::IRListView<false> *IR);
      void CalculateNodeInterference(FEXCore::IR::IRListView<false> *IR);
      void AllocateVirtualRegisters();

      FEXCore::IR::AllNodesIterator FindFirstUse(FEXCore::IR::IREmitter *IREmit, FEXCore::IR::OrderedNode* Node, FEXCore::IR::AllNodesIterator Begin, FEXCore::IR::AllNodesIterator End);
      FEXCore::IR::AllNodesIterator FindLastUseBefore(FEXCore::IR::IREmitter *IREmit, FEXCore::IR::OrderedNode* Node, FEXCore::IR::AllNodesIterator Begin, FEXCore::IR::AllNodesIterator End);

      uint32_t FindNodeToSpill(IREmitter *IREmit, RegisterNode *RegisterNode, uint32_t CurrentLocation, LiveRange const *OpLiveRange, int32_t RematCost = -1);
      uint32_t FindSpillSlot(uint32_t Node, FEXCore::IR::RegisterClassType RegisterClass);

      bool RunAllocateVirtualRegisters(IREmitter *IREmit);
  };

  ConstrainedRAPass::ConstrainedRAPass() {
    LocalCompaction.reset(FEXCore::IR::CreateIRCompaction());
  }

  ConstrainedRAPass::~ConstrainedRAPass() {
    FreeRegisterGraph(Graph);
  }

  void ConstrainedRAPass::AllocateRegisterSet(uint32_t RegisterCount, uint32_t ClassCount) {
    // We don't care about Max register count
    PhysicalRegisterCount.resize(ClassCount);
    TopRAPressure.resize(ClassCount);

    Graph = AllocateRegisterGraph(ClassCount);
  }

  void ConstrainedRAPass::AddRegisters(FEXCore::IR::RegisterClassType Class, uint32_t RegisterCount) {
    AllocateRegisters(Graph, Class, DEFAULT_VIRTUAL_REG_COUNT);
    AllocatePhysicalRegisters(Graph, Class, RegisterCount);
    PhysicalRegisterCount[Class] = RegisterCount;
  }

  void ConstrainedRAPass::AddRegisterConflict(FEXCore::IR::RegisterClassType ClassConflict, uint32_t RegConflict, FEXCore::IR::RegisterClassType Class, uint32_t Reg) {
    VirtualAddRegisterConflict(Graph, ClassConflict, RegConflict, Class, Reg);
  }

  void ConstrainedRAPass::AllocateRegisterConflicts(FEXCore::IR::RegisterClassType Class, uint32_t NumConflicts) {
    VirtualAllocateRegisterConflicts(Graph, Class, NumConflicts);
  }

  uint64_t ConstrainedRAPass::GetNodeRegister(uint32_t Node) {
    return Graph->Nodes[Node].Head.RegAndClass;
  }

  void ConstrainedRAPass::CalculateLiveRange(FEXCore::IR::IRListView<false> *IR) {
    using namespace FEXCore;
    size_t Nodes = IR->GetSSACount();
    if (Nodes > LiveRanges.size()) {
      LiveRanges.resize(Nodes);
    }
    LiveRanges.assign(Nodes * sizeof(LiveRange), {~0U, ~0U});

    constexpr uint32_t DEFAULT_REMAT_COST = 1000;
    for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
      for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
        uint32_t Node = IR->GetID(CodeNode);

        // If the destination hasn't yet been set then set it now
        if (IROp->HasDest) {
          LogMan::Throw::A(LiveRanges[Node].Begin == ~0U, "Node begin already defined?");
          LiveRanges[Node].Begin = Node;
          // Default to ending right where it starts
          LiveRanges[Node].End = Node;
        }

        // Calculate remat cost
        switch (IROp->Op) {
          case IR::OP_CONSTANT: LiveRanges[Node].RematCost = 1; break;
          case IR::OP_LOADFLAG:
          case IR::OP_LOADCONTEXT: LiveRanges[Node].RematCost = 10; break;
          case IR::OP_LOADMEM:
          case IR::OP_LOADMEMTSO:
            LiveRanges[Node].RematCost = 100;
            break;
          case IR::OP_FILLREGISTER: LiveRanges[Node].RematCost = DEFAULT_REMAT_COST + 1; break;
          // We want PHI to be very expensive to spill
          case IR::OP_PHI: LiveRanges[Node].RematCost = DEFAULT_REMAT_COST * 10; break;
          default: LiveRanges[Node].RematCost = DEFAULT_REMAT_COST; break;
        }

        // Set this node's block ID
        Graph->Nodes[Node].Head.BlockID = IR->GetID(BlockNode);

        uint8_t NumArgs = IR::GetArgs(IROp->Op);
        for (uint8_t i = 0; i < NumArgs; ++i) {
          if (IROp->Args[i].IsInvalid()) continue;
          uint32_t ArgNode = IROp->Args[i].ID();
          // Set the node end to be at least here
          LiveRanges[ArgNode].End = Node;
          LogMan::Throw::A(LiveRanges[ArgNode].Begin != ~0U, "%%ssa%d used by %%ssa%d before defined?", ArgNode, Node);
        }

        if (IROp->Op == IR::OP_PHI) {
          // Special case the PHI op, all of the nodes in the argument need to have the same virtual register affinity
          // Walk through all of them and set affinities for each other
          auto Op = IROp->C<IR::IROp_Phi>();
          auto NodeBegin = IR->at(Op->PhiBegin);

          uint32_t CurrentSourcePartner = Node;
          while (NodeBegin != NodeBegin.Invalid()) {
            auto [ValueNode, ValueHeader] = NodeBegin();
            auto ValueOp = ValueHeader->CW<IROp_PhiValue>();

            // Set the node partner to the current one
            // This creates a singly linked list of node partners to follow
            SetNodePartner(Graph, CurrentSourcePartner, ValueOp->Value.ID());
            CurrentSourcePartner = ValueOp->Value.ID();
            NodeBegin = IR->at(ValueOp->Next);
          }
        }
      }
    }
  }

  void ConstrainedRAPass::CalculateBlockInterferences(FEXCore::IR::IRListView<false> *IR) {
    using namespace FEXCore;

    for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
      auto BlockIROp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();
      LogMan::Throw::A(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

      BlockInterferences *BlockInterferenceVector = &LocalBlockInterferences.try_emplace(IR->GetID(BlockNode)).first->second;
      BlockInterferenceVector->reserve(BlockIROp->Last.ID() - BlockIROp->Begin.ID());

      for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
        uint32_t Node = IR->GetID(CodeNode);
        LiveRange *NodeLiveRange = &LiveRanges[Node];

        if (NodeLiveRange->Begin >= BlockIROp->Begin.ID() &&
            NodeLiveRange->End <= BlockIROp->Last.ID()) {
          // If the live range of this node is FULLY inside of the block
          // Then add it to the block specific interference list
          BlockInterferenceVector->emplace_back(Node);
        }
        else {
          // If the live range is not fully inside the block then add it to the global interference list
          GlobalBlockInterferences.emplace_back(Node);
        }
      }
    }
  }

  void ConstrainedRAPass::CalculateBlockNodeInterference(FEXCore::IR::IRListView<false> *IR) {
    auto AddInterference = [&](uint32_t Node1, uint32_t Node2) {
      RegisterNode *Node = &Graph->Nodes[Node1];
      Node->Interference.Set(Node2);
      Node->InterferenceList[Node->Head.InterferenceCount++] = Node2;
    };

    auto CheckInterferenceNodeSizes = [&](uint32_t Node1, uint32_t MaxNewNodes) {
      RegisterNode *Node = &Graph->Nodes[Node1];
      uint32_t NewListMax = Node->Head.InterferenceCount + MaxNewNodes;
      if (Node->InterferenceListSize <= NewListMax) {
        Node->InterferenceListSize = std::max(Node->InterferenceListSize * 2U, (uint32_t)AlignUp(NewListMax, DEFAULT_INTERFERENCE_LIST_COUNT));
        Node->InterferenceList = reinterpret_cast<uint32_t*>(realloc(Node->InterferenceList, Node->InterferenceListSize * sizeof(uint32_t)));
      }
    };
    using namespace FEXCore;

    for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
      BlockInterferences *BlockInterferenceVector = &LocalBlockInterferences.try_emplace(IR->GetID(BlockNode)).first->second;

      std::vector<uint32_t> Interferences;
      Interferences.reserve(BlockInterferenceVector->size() + GlobalBlockInterferences.size());

      for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
        uint32_t Node = IR->GetID(CodeNode);

        // Check for every interference with the local block's interference
        for (auto RHSNode : *BlockInterferenceVector) {
          if (!(LiveRanges[Node].Begin >= LiveRanges[RHSNode].End ||
                LiveRanges[RHSNode].Begin >= LiveRanges[Node].End)) {
            Interferences.emplace_back(RHSNode);
          }
        }

        // Now check the global block interference vector
        for (auto RHSNode : GlobalBlockInterferences) {
          if (!(LiveRanges[Node].Begin >= LiveRanges[RHSNode].End ||
                LiveRanges[RHSNode].Begin >= LiveRanges[Node].End)) {
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
  }

  void ConstrainedRAPass::CalculateNodeInterference(FEXCore::IR::IRListView<false> *IR) {
    auto AddInterference = [&](uint32_t Node1, uint32_t Node2) {
      RegisterNode *Node = &Graph->Nodes[Node1];
      Node->Interference.Set(Node2);
      Node->InterferenceList[Node->Head.InterferenceCount++] = Node2;
      if (Node->InterferenceListSize <= Node->Head.InterferenceCount) {
        Node->InterferenceListSize *= 2;
        Node->InterferenceList = reinterpret_cast<uint32_t*>(realloc(Node->InterferenceList, Node->InterferenceListSize * sizeof(uint32_t)));
      }
    };

    uint32_t NodeCount = IR->GetSSACount();

    // Now that we have all the live ranges calculated we need to add them to our interference graph
    for (uint32_t i = 0; i < NodeCount; ++i) {
      for (uint32_t j = i + 1; j < NodeCount; ++j) {
        if (!(LiveRanges[i].Begin >= LiveRanges[j].End ||
              LiveRanges[j].Begin >= LiveRanges[i].End)) {
          AddInterference(i, j);
          AddInterference(j, i);
        }
      }
    }
  }

  void ConstrainedRAPass::AllocateVirtualRegisters() {
    for (uint32_t i = 0; i < Graph->NodeCount; ++i) {
      RegisterNode *CurrentNode = &Graph->Nodes[i];
      if (CurrentNode->Head.RegAndClass == INVALID_REGCLASS)
        continue;

      FEXCore::IR::RegisterClassType RegClass = FEXCore::IR::RegisterClassType{uint32_t(CurrentNode->Head.RegAndClass >> 32)};
      uint64_t RegAndClass = ~0ULL;
      RegisterClass *RAClass = &Graph->Set.Classes[RegClass];
      if (CurrentNode->Head.PhiPartner) {
        // In the case that we have a list of nodes that need the same register allocated we need to do something special
        // We need to gather the data from the forward linked list and make sure they all match the virtual register
        std::vector<RegisterNode *> Nodes;
        auto CurrentPartner = CurrentNode;
        while (CurrentPartner) {
          Nodes.emplace_back(CurrentPartner);
          CurrentPartner = CurrentPartner->Head.PhiPartner;
        }

        for (uint32_t ri = 0; ri < RAClass->Count; ++ri) {
          uint64_t RegisterToCheck = (static_cast<uint64_t>(RegClass) << 32) + ri;
          if (!DoesNodeSetInterfereWithRegister(Graph, Nodes, RegisterToCheck)) {
            RegAndClass = RegisterToCheck;
            break;
          }
        }

        // If we failed to find a virtual register then allocate more space for them
        if (RegAndClass == ~0ULL) {
          RegAndClass = (static_cast<uint64_t>(RegClass.Val) << 32);
          RegAndClass |= AllocateMoreRegisters(Graph, RegClass);
        }

        TopRAPressure[RegClass] = std::max((uint32_t)RegAndClass, TopRAPressure[RegClass]);

        // Walk the partners and ensure they are all set to the same register now
        for (auto Partner : Nodes) {
          Partner->Head.RegAndClass = RegAndClass;
        }
      }
      else {
        for (uint32_t ri = 0; ri < RAClass->Count; ++ri) {
          uint64_t RegisterToCheck = (static_cast<uint64_t>(RegClass) << 32) + ri;
          if (!DoesNodeInterfereWithRegister(Graph, CurrentNode, RegisterToCheck)) {
            RegAndClass = RegisterToCheck;
            break;
          }
        }

        // If we failed to find a virtual register then allocate more space for them
        if (RegAndClass == ~0ULL) {
          RegAndClass = (static_cast<uint64_t>(RegClass.Val) << 32);
          RegAndClass |= AllocateMoreRegisters(Graph, RegClass);
        }

        TopRAPressure[RegClass] = std::max((uint32_t)RegAndClass, TopRAPressure[RegClass]);
        CurrentNode->Head.RegAndClass = RegAndClass;
      }
    }
  }

  FEXCore::IR::AllNodesIterator ConstrainedRAPass::FindFirstUse(FEXCore::IR::IREmitter *IREmit, FEXCore::IR::OrderedNode* Node, FEXCore::IR::AllNodesIterator Begin, FEXCore::IR::AllNodesIterator End) {
    using namespace FEXCore::IR;
    uint32_t SearchID = IREmit->ViewIR().GetID(Node);

    while(1) {
      auto [RealNode, IROp] = Begin();

      uint8_t NumArgs = FEXCore::IR::GetArgs(IROp->Op);
      for (uint8_t i = 0; i < NumArgs; ++i) {
        uint32_t ArgNode = IROp->Args[i].ID();
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
    uint32_t SearchID = CurrentIR.GetID(Node);

    while (1) {
      using namespace FEXCore::IR;
      auto [RealNode, IROp] = End();

      if (Node == RealNode) {
        // We walked back all the way to the definition of the IR op
        return End;
      }

      uint8_t NumArgs = FEXCore::IR::GetArgs(IROp->Op);
      for (uint8_t i = 0; i < NumArgs; ++i) {
        uint32_t ArgNode = IROp->Args[i].ID();
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

  uint32_t ConstrainedRAPass::FindNodeToSpill(IREmitter *IREmit, RegisterNode *RegisterNode, uint32_t CurrentLocation, LiveRange const *OpLiveRange, int32_t RematCost) {
    auto IR = IREmit->ViewIR();

    uint32_t InterferenceToSpill = ~0U;
    uint32_t InterferenceFarthestNextUse = 0;

    IR::OrderedNodeWrapper NodeOpBegin = IR::OrderedNodeWrapper::WrapOffset(CurrentLocation * sizeof(IR::OrderedNode));
    IR::OrderedNodeWrapper NodeOpEnd = IR::OrderedNodeWrapper::WrapOffset(OpLiveRange->End * sizeof(IR::OrderedNode));
    auto NodeOpBeginIter = IR.at(NodeOpBegin);
    auto NodeOpEndIter = IR.at(NodeOpEnd);

    // Couldn't find register to spill
    // Be more aggressive
    if (InterferenceToSpill == ~0U) {
      for (uint32_t j = 0; j < RegisterNode->Head.InterferenceCount; ++j) {
        uint32_t InterferenceNode = RegisterNode->InterferenceList[j];
        auto *InterferenceLiveRange = &LiveRanges[InterferenceNode];
        if (InterferenceLiveRange->RematCost == -1 ||
            (RematCost != -1 && InterferenceLiveRange->RematCost != RematCost)) {
          continue;
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
          continue;
        }

        auto [InterferenceOrderedNode, _] = IR.at(InterferenceNode)();
        auto InterferenceNodeOpBeginIter = IR.at(InterferenceLiveRange->Begin);
        auto InterferenceNodeOpEndIter = IR.at(InterferenceLiveRange->End);

        bool Found{};

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
            LogMan::Throw::A(InterferenceNodeNextUse != IR::NodeIterator::Invalid(), "Couldn't find next usage of op");
            // If there is no use of the interference op prior to our op then it only has initial definition
            if (InterferenceNodePrevUse == IR::NodeIterator::Invalid()) InterferenceNodePrevUse = InterferenceNodeOpBeginIter;

            uint32_t NextUseDistance = InterferenceNodeNextUse.ID() - CurrentLocation;
            if (NextUseDistance >= InterferenceFarthestNextUse) {
              Found = true;
              InterferenceToSpill = j;
              InterferenceFarthestNextUse = NextUseDistance;
            }
          }
        }
      }
    }

    if (InterferenceToSpill == ~0U) {
      for (uint32_t j = 0; j < RegisterNode->Head.InterferenceCount; ++j) {
        uint32_t InterferenceNode = RegisterNode->InterferenceList[j];
        auto *InterferenceLiveRange = &LiveRanges[InterferenceNode];
        if (InterferenceLiveRange->RematCost == -1 ||
            (RematCost != -1 && InterferenceLiveRange->RematCost != RematCost)) {
          continue;
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
          continue;
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
            auto InterferenceNodeNextUse = FindFirstUse(IREmit, InterferenceOrderedNode, NodeOpBeginIter, InterferenceNodeOpEndIter);
            uint32_t NextUseDistance = InterferenceNodeNextUse.ID() - CurrentLocation;
            if (NextUseDistance >= InterferenceFarthestNextUse) {
              Found = true;

              InterferenceToSpill = j;
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
            auto InterferenceNodeNextUse = FindFirstUse(IREmit, InterferenceOrderedNode, NodeOpBeginIter, InterferenceNodeOpEndIter);
            uint32_t NextUseDistance = InterferenceNodeNextUse.ID() - CurrentLocation;
            if (NextUseDistance >= InterferenceFarthestNextUse) {
              Found = true;

              InterferenceToSpill = j;
              InterferenceFarthestNextUse = NextUseDistance;
            }
          }
        }
      }
    }

    // If we are looking for a specific node then we can safely return not found
    if (RematCost != -1 && InterferenceToSpill == ~0U) {
      return ~0U;
    }

    if (InterferenceToSpill == ~0U) {
      LogMan::Msg::D("node %%ssa%d has %ld interferences, was dumped in to virtual reg %d. Live Range[%d, %d)",
        CurrentLocation, RegisterNode->Head.InterferenceCount, RegisterNode->Head.RegAndClass,
        OpLiveRange->Begin, OpLiveRange->End);
      for (uint32_t j = 0; j < RegisterNode->Head.InterferenceCount; ++j) {
        uint32_t InterferenceNode = RegisterNode->InterferenceList[j];
        auto *InterferenceLiveRange = &LiveRanges[InterferenceNode];

        LogMan::Msg::D("\tInt%d: %%ssa%d Remat: %d [%d, %d)", j, InterferenceNode, InterferenceLiveRange->RematCost, InterferenceLiveRange->Begin, InterferenceLiveRange->End);
      }
    }
    LogMan::Throw::A(InterferenceToSpill != ~0U, "Couldn't find Node to spill");

    return RegisterNode->InterferenceList[InterferenceToSpill];
  }

  uint32_t ConstrainedRAPass::FindSpillSlot(uint32_t Node, FEXCore::IR::RegisterClassType RegisterClass) {
    RegisterNode *CurrentNode = &Graph->Nodes[Node];
    LiveRange *NodeLiveRange = &LiveRanges[Node];
    for (uint32_t i = 0; i < Graph->SpillStack.size(); ++i) {
      SpillStackUnit *SpillUnit = &Graph->SpillStack.at(i);
      if (NodeLiveRange->Begin <= SpillUnit->SpillRange.End &&
          SpillUnit->SpillRange.Begin <= NodeLiveRange->End) {
        SpillUnit->SpillRange.Begin = std::min(SpillUnit->SpillRange.Begin, LiveRanges[Node].Begin);
        SpillUnit->SpillRange.End = std::max(SpillUnit->SpillRange.End, LiveRanges[Node].End);
        CurrentNode->Head.SpillSlot = i;
        return i;
      }
    }

    // Couldn't find a spill slot so just make a new one
    auto StackItem = Graph->SpillStack.emplace_back(SpillStackUnit{Node, RegisterClass});
    StackItem.SpillRange.Begin = NodeLiveRange->Begin;
    StackItem.SpillRange.End = NodeLiveRange->End;
    CurrentNode->Head.SpillSlot = SpillSlotCount;
    SpillSlotCount++;
    return CurrentNode->Head.SpillSlot;
  }

  void ConstrainedRAPass::SpillRegisters(FEXCore::IR::IREmitter *IREmit) {
    using namespace FEXCore;

    auto IR = IREmit->ViewIR();
    auto LastCursor = IREmit->GetWriteCursor();

    for (auto [BlockNode, BlockIRHeader] : IR.GetBlocks()) {
      for (auto [CodeNode, IROp] : IR.GetCode(BlockNode)) {

        if (IROp->HasDest) {
          uint32_t Node = IR.GetID(CodeNode);
          RegisterNode *CurrentNode = &Graph->Nodes[Node];
          LiveRange *OpLiveRange = &LiveRanges[Node];

          // If this node is allocated above the number of physical registers we have then we need to search the interference list and spill the one
          // that is cheapest
          FEXCore::IR::RegisterClassType RegClass = FEXCore::IR::RegisterClassType{uint32_t(CurrentNode->Head.RegAndClass >> 32)};
          bool NeedsToSpill = (uint32_t)CurrentNode->Head.RegAndClass >= PhysicalRegisterCount.at(RegClass);

          if (NeedsToSpill) {
            bool Spilled = false;

            // First let's just check for constants that we can just rematerialize instead of spilling
            uint32_t InterferenceNode = FindNodeToSpill(IREmit, CurrentNode, Node, OpLiveRange, 1);
            if (InterferenceNode != ~0U) {
              // We want to end the live range of this value here and continue it on first use
              auto [ConstantNode, _] = IR.at(InterferenceNode)();
              auto ConstantIROp = IR.GetOp<IR::IROp_Constant>(ConstantNode);

              // First op post Spill
              auto NextIter = IR.at(CodeNode);
              auto FirstUseLocation = FindFirstUse(IREmit, ConstantNode, NextIter, NodeIterator::Invalid());
              LogMan::Throw::A(FirstUseLocation != IR::NodeIterator::Invalid(), "At %%ssa%d Spilling Op %%ssa%d but Failure to find op use", Node, InterferenceNode);
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
              uint32_t InterferenceNode = FindNodeToSpill(IREmit, CurrentNode, Node, OpLiveRange);
              if (InterferenceNode != ~0U) {
                FEXCore::IR::RegisterClassType InterferenceRegClass = FEXCore::IR::RegisterClassType{uint32_t(Graph->Nodes[InterferenceNode].Head.RegAndClass >> 32)};
                uint32_t SpillSlot = FindSpillSlot(InterferenceNode, InterferenceRegClass);
                RegisterNode *InterferenceRegisterNode = &Graph->Nodes[InterferenceNode];
                LogMan::Throw::A(SpillSlot != ~0U, "Interference Node doesn't have a spill slot!");
                LogMan::Throw::A((InterferenceRegisterNode->Head.RegAndClass & ~0U) != ~0U, "Interference node never assigned a register?");
                LogMan::Throw::A(InterferenceRegClass != ~0U, "Interference node never assigned a register class?");
                LogMan::Throw::A(InterferenceRegisterNode->Head.PhiPartner == nullptr, "We don't support spilling PHI nodes currently");

                // This is the op that we need to dump
                auto [InterferenceOrderedNode, InterferenceIROp] = IR.at(InterferenceNode)();


                // This will find the last use of this definition
                // Walks from CodeBegin -> BlockBegin to find the last Use
                // Which this is walking backwards to find the first use
                auto LastUseIterator = FindLastUseBefore(IREmit, InterferenceOrderedNode, NodeIterator::Invalid(), IR.at(CodeNode));
                auto [LastUseNode, LastUseIROp] = LastUseIterator();

                // Set the write cursor to point of last usage
                IREmit->SetWriteCursor(LastUseNode);

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

                  LogMan::Throw::A(FirstUseLocation != NodeIterator::Invalid(), "At %%ssa%d Spilling Op %%ssa%d but Failure to find op use", Node, InterferenceNode);
                  if (FirstUseLocation != IR::NodeIterator::Invalid()) {
                    // We want to fill just before the first use
                    --FirstUseLocation;
                    auto [FirstUseOrderedNode, _] = FirstUseLocation();

                    IREmit->SetWriteCursor(FirstUseOrderedNode);

                    auto FilledInterference = IREmit->_FillRegister(SpillSlot, InterferenceRegClass);
                    FilledInterference.first->Header.Size = InterferenceIROp->Size;
                    FilledInterference.first->Header.ElementSize = InterferenceIROp->ElementSize;
                    IREmit->ReplaceUsesWithAfter(InterferenceOrderedNode, FilledInterference, FirstUseLocation);
                    Spilled = true;
                  }
                }
              }
            }

            IREmit->SetWriteCursor(LastCursor);
            // We can't spill multiple times in a row. Need to restart
            if (Spilled) {
              return;
            }
          }
        }
      }
    }
  }

  bool ConstrainedRAPass::RunAllocateVirtualRegisters(FEXCore::IR::IREmitter *IREmit) {
    using namespace FEXCore;
    bool Changed = false;

    GlobalBlockInterferences.clear();
    LocalBlockInterferences.clear();

    TopRAPressure.assign(TopRAPressure.size(), 0);

    // We need to rerun compaction every step
    Changed |= LocalCompaction->Run(IREmit);
    auto IR = IREmit->ViewIR();

    uint32_t SSACount = IR.GetSSACount();

    ResetRegisterGraph(Graph, SSACount);
    FindNodeClasses(Graph, &IR);
    CalculateLiveRange(&IR);

    // Linear foward scan based interference calculation is faster for smaller blocks
    // Smarter block based interference calculation is faster for larger blocks
    if (SSACount >= 2048) {
      CalculateBlockInterferences(&IR);
      CalculateBlockNodeInterference(&IR);
    }
    else {
      CalculateNodeInterference(&IR);
    }
    AllocateVirtualRegisters();

    return Changed;
  }


  bool ConstrainedRAPass::Run(IREmitter *IREmit) {
    bool Changed = false;

    auto IR = IREmit->ViewIR();

    auto HeaderOp = IR.GetHeader();
    if (HeaderOp->ShouldInterpret) {
      return false;
    }

    SpillSlotCount = 0;
    Graph->SpillStack.clear();

    while (1) {
      HadFullRA = true;

      // Virtual allocation pass runs the compaction pass per run
      Changed |= RunAllocateVirtualRegisters(IREmit);

      for (size_t i = 0; i < PhysicalRegisterCount.size(); ++i) {
        // Virtual registers fit completely within physical registers
        // Remap virtual 1:1 to physical
        HadFullRA &= TopRAPressure[i] < PhysicalRegisterCount[i];
      }

      if (HadFullRA) {
        break;
      }

      SpillRegisters(IREmit);
      Changed = true;
    }

    return Changed;
  }

  FEXCore::IR::RegisterAllocationPass* CreateRegisterAllocationPass() {
    return new ConstrainedRAPass{};
  }
}
