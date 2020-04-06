#include "Common/BitSet.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"
#include "Interface/IR/Passes.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <iterator>

namespace {
  constexpr uint32_t INVALID_REG = ~0U;
  constexpr uint32_t INVALID_CLASS = ~0U;
  constexpr uint32_t DEFAULT_INTERFERENCE_LIST_COUNT = 128;
  constexpr uint32_t DEFAULT_NODE_COUNT = 8192;
  constexpr uint32_t DEFAULT_VIRTUAL_REG_COUNT = 1024;

  struct Register {
    bool Virtual;
    uint64_t Index;
  };

  struct RegisterClass {
    uint32_t NumberOfRegisters;
  };

  struct RegisterNode {
    struct VolatileHeader {
      uint32_t RegisterClass;
      uint32_t Register;
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
    .RegisterClass = INVALID_CLASS,
    .Register = INVALID_REG,
    .InterferenceCount = 0,
    .BlockID = ~0U,
    .SpillSlot = ~0U,
    .PhiPartner = nullptr,
  };

  struct RegisterSet {
    RegisterClass *Classes;
    uint32_t ClassCount;
  };

  struct LiveRange {
    uint32_t Begin;
    uint32_t End;
    uint32_t RematCost;
  };

  struct SpillStackUnit {
    uint32_t Node;
    uint32_t Class;
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
    Graph->Set.Classes = new RegisterClass[ClassCount];

    // Allocate default nodes
    ResetRegisterGraph(Graph, DEFAULT_NODE_COUNT);
    return Graph;
  }

  void AllocateRegisters(RegisterGraph *Graph, uint32_t Class, uint32_t Count) {
    Graph->Set.Classes[Class].NumberOfRegisters = Count;
  }

  // Returns the new register ID that was the previous top
  uint32_t AllocateMoreRegisters(RegisterGraph *Graph, uint32_t Class) {
    RegisterClass &LocalClass = Graph->Set.Classes[Class];
    uint32_t OldNumber = LocalClass.NumberOfRegisters;
    LocalClass.NumberOfRegisters *= 2;
    return OldNumber;
  }

  void FreeRegisterGraph(RegisterGraph *Graph) {
    for (size_t i = 0; i <Graph->MaxNodeCount; ++i) {
      free(Graph->Nodes[i].InterferenceList);
    }
    free(Graph->Nodes);
    Graph->InterferenceSet.Free();

    delete[] Graph->Set.Classes;
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

  void SetNodeClass(RegisterGraph *Graph, uint32_t Node, uint32_t Class) {
    Graph->Nodes[Node].Head.RegisterClass = Class;
  }
  void SetNodePartner(RegisterGraph *Graph, uint32_t Node, uint32_t Partner) {
    Graph->Nodes[Node].Head.PhiPartner = &Graph->Nodes[Partner];
  }

  bool DoesNodeInterfereWithRegister(RegisterGraph *Graph, RegisterNode const *Node, uint32_t Register) {
    // Walk the node's interference list and see if it interferes with this register
    for (uint32_t i = 0; i < Node->Head.InterferenceCount; ++i) {
      RegisterNode *InterferenceNode = &Graph->Nodes[Node->InterferenceList[i]];
      if (InterferenceNode->Head.Register == Register) {
        return true;
      }
    }
    return false;
  }

  bool DoesNodeSetInterfereWithRegister(RegisterGraph *Graph, std::vector<RegisterNode*> const &Nodes, uint32_t Register) {
    for (auto it : Nodes) {
      if (DoesNodeInterfereWithRegister(Graph, it, Register)) {
        return true;
      }
    }

    return false;
  }

  uint32_t GetRegClassFromNode(uintptr_t ListBegin, uintptr_t DataBegin, FEXCore::IR::OrderedNodeWrapper const WrapperOp) {
    using namespace FEXCore;
    IR::OrderedNode const *RealNode = WrapperOp.GetNode(ListBegin);
    IR::IROp_Header const *IROp = RealNode->Op(DataBegin);

    // XXX: This needs to be better
    switch (IROp->Op) {
    case IR::OP_LOADCONTEXT: {
      auto Op = IROp->C<IR::IROp_LoadContext>();
      if (Op->Class.Val == 1)
        return IR::RegisterAllocationPass::FPRClass;
      else
        return IR::RegisterAllocationPass::GPRClass;
      break;
    }
    case IR::OP_STORECONTEXT: {
      auto Op = IROp->C<IR::IROp_StoreContext>();
      if (Op->Class.Val == 1)
        return IR::RegisterAllocationPass::FPRClass;
      else
        return IR::RegisterAllocationPass::GPRClass;
      break;
    }
    case IR::OP_LOADMEM: {
      auto Op = IROp->C<IR::IROp_LoadMem>();
      if (Op->Class.Val == 1)
        return IR::RegisterAllocationPass::FPRClass;
      else
        return IR::RegisterAllocationPass::GPRClass;
      break;
    }
    case IR::OP_STOREMEM: {
      auto Op = IROp->C<IR::IROp_StoreMem>();
      if (Op->Class.Val == 1)
        return IR::RegisterAllocationPass::FPRClass;
      else
        return IR::RegisterAllocationPass::GPRClass;
      break;
    }
    case IR::OP_ZEXT: {
      auto Op = IROp->C<IR::IROp_Zext>();
      LogMan::Throw::A(Op->SrcSize <= 64, "Can't support Zext of size: %ld", Op->SrcSize);

      if (Op->SrcSize == 64) {
        return IR::RegisterAllocationPass::FPRClass;
      }
      else {
        return IR::RegisterAllocationPass::GPRClass;
      }
      break;
    }
    case IR::OP_CPUID: return IR::RegisterAllocationPass::FPRClass; break;
    case IR::OP_PHIVALUE: {
      // Unwrap the PHIValue to get the class
      auto Op = IROp->C<IR::IROp_PhiValue>();
      return GetRegClassFromNode(ListBegin, DataBegin, Op->Value);
    }
    case IR::OP_PHI: {
      // Class is defined from the values passed in
      // All Phi nodes should have its class be the same (Validation should confirm this
      auto Op = IROp->C<IR::IROp_Phi>();
      return GetRegClassFromNode(ListBegin, DataBegin, Op->PhiBegin);
    }
    default:
      if (IROp->Op >= IR::OP_GETHOSTFLAG)
        return IR::RegisterAllocationPass::FLAGSClass;
      else if (IROp->Op > IR::OP_PRINT)
        return IR::RegisterAllocationPass::FPRClass;
      else
        return IR::RegisterAllocationPass::GPRClass;
      break;
    }

    // Unreachable
    return IR::RegisterAllocationPass::GPRClass;
  };

  // Walk the IR and set the node classes
  void FindNodeClasses(RegisterGraph *Graph, FEXCore::IR::IRListView<false> *IR) {
    uintptr_t ListBegin = IR->GetListData();
    uintptr_t DataBegin = IR->GetData();

    auto Begin = IR->begin();
    auto Op = Begin();

    FEXCore::IR::OrderedNode *RealNode = Op->GetNode(ListBegin);
    auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
    LogMan::Throw::A(HeaderOp->Header.Op == FEXCore::IR::OP_IRHEADER, "First op wasn't IRHeader");

    FEXCore::IR::OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

    while (1) {
      auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
      LogMan::Throw::A(BlockIROp->Header.Op == FEXCore::IR::OP_CODEBLOCK, "IR type failed to be a code block");

      // We grab these nodes this way so we can iterate easily
      auto CodeBegin = IR->at(BlockIROp->Begin);
      auto CodeLast = IR->at(BlockIROp->Last);
      while (1) {
        FEXCore::IR::OrderedNodeWrapper *CodeOp = CodeBegin();
        FEXCore::IR::OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
        auto IROp = CodeNode->Op(DataBegin);
        uint32_t Node = CodeOp->ID();

        // If the destination hasn't yet been set then set it now
        if (IROp->HasDest) {
          SetNodeClass(Graph, Node, GetRegClassFromNode(ListBegin, DataBegin, *CodeOp));
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
}

namespace FEXCore::IR {
  class ConstrainedRAPass final : public RegisterAllocationPass {
    public:
      ConstrainedRAPass();
      ~ConstrainedRAPass();
      bool Run(OpDispatchBuilder *Disp) override;

      void AllocateRegisterSet(uint32_t RegisterCount, uint32_t ClassCount) override;
      void AddRegisters(uint32_t Class, uint32_t RegisterCount) override;

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

      void SpillRegisters(FEXCore::IR::OpDispatchBuilder *Disp);

      std::vector<LiveRange> LiveRanges;

      using BlockInterferences = std::vector<uint32_t>;

      std::unordered_map<uint32_t, BlockInterferences> LocalBlockInterferences;
      BlockInterferences GlobalBlockInterferences;

      void CalculateLiveRange(FEXCore::IR::IRListView<false> *IR);
      void CalculateBlockInterferences(FEXCore::IR::IRListView<false> *IR);
      void CalculateBlockNodeInterference(FEXCore::IR::IRListView<false> *IR);
      void CalculateNodeInterference(FEXCore::IR::IRListView<false> *IR);
      void AllocateVirtualRegisters();

      FEXCore::IR::NodeWrapperIterator FindFirstUse(FEXCore::IR::OpDispatchBuilder *Disp, FEXCore::IR::OrderedNode* Node, FEXCore::IR::NodeWrapperIterator Begin, FEXCore::IR::NodeWrapperIterator End);
      uint32_t FindNodeToSpill(RegisterNode *RegisterNode, uint32_t CurrentLocation, LiveRange const *OpLiveRange);
      uint32_t FindSpillSlot(uint32_t Node, uint32_t RegisterClass);

      bool RunAllocateVirtualRegisters(OpDispatchBuilder *Disp);
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
    for (size_t i = 0; i < ClassCount; ++i) {
      AllocateRegisters(Graph, i, DEFAULT_VIRTUAL_REG_COUNT);
    }
  }

  void ConstrainedRAPass::AddRegisters(uint32_t Class, uint32_t RegisterCount) {
    PhysicalRegisterCount[Class] = RegisterCount;
  }

  uint64_t ConstrainedRAPass::GetNodeRegister(uint32_t Node) {
    uint64_t Reg = (static_cast<uint64_t>(Graph->Nodes[Node].Head.RegisterClass) << 32) | Graph->Nodes[Node].Head.Register;
    return Reg;
  }

  void ConstrainedRAPass::CalculateLiveRange(FEXCore::IR::IRListView<false> *IR) {
    using namespace FEXCore;
    size_t Nodes = IR->GetSSACount();
    if (Nodes > LiveRanges.size()) {
      LiveRanges.resize(Nodes);
    }
    LiveRanges.assign(Nodes * sizeof(LiveRange), {~0U, ~0U});

    uintptr_t ListBegin = IR->GetListData();
    uintptr_t DataBegin = IR->GetData();

    auto Begin = IR->begin();
    auto Op = Begin();

    IR::OrderedNode *RealNode = Op->GetNode(ListBegin);
    auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
    LogMan::Throw::A(HeaderOp->Header.Op == IR::OP_IRHEADER, "First op wasn't IRHeader");

    IR::OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

    constexpr uint32_t DEFAULT_REMAT_COST = 1000;
    while (1) {
      auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
      LogMan::Throw::A(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

      // We grab these nodes this way so we can iterate easily
      auto CodeBegin = IR->at(BlockIROp->Begin);
      auto CodeLast = IR->at(BlockIROp->Last);
      while (1) {
        auto CodeOp = CodeBegin();
        IR::OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
        auto IROp = CodeNode->Op(DataBegin);
        uint32_t Node = CodeOp->ID();

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
          case IR::OP_LOADMEM: LiveRanges[Node].RematCost = 100; break;
          case IR::OP_FILLREGISTER: LiveRanges[Node].RematCost = DEFAULT_REMAT_COST + 1; break;
          // We want PHI to be very expensive to spill
          case IR::OP_PHI: LiveRanges[Node].RematCost = DEFAULT_REMAT_COST * 10; break;
          default: LiveRanges[Node].RematCost = DEFAULT_REMAT_COST; break;
        }

        // Set this node's block ID
        Graph->Nodes[Node].Head.BlockID = BlockNode->Wrapped(ListBegin).ID();

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
            FEXCore::IR::OrderedNodeWrapper *NodeOp = NodeBegin();
            FEXCore::IR::OrderedNode *NodeNode = NodeOp->GetNode(ListBegin);
            auto IRNodeOp = NodeNode->Op(DataBegin)->C<IR::IROp_PhiValue>();

            // Set the node partner to the current one
            // This creates a singly linked list of node partners to follow
            SetNodePartner(Graph, CurrentSourcePartner, IRNodeOp->Value.ID());
            CurrentSourcePartner = IRNodeOp->Value.ID();
            NodeBegin = IR->at(IRNodeOp->Next);
          }
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

  void ConstrainedRAPass::CalculateBlockInterferences(FEXCore::IR::IRListView<false> *IR) {
    using namespace FEXCore;
    uintptr_t ListBegin = IR->GetListData();
    uintptr_t DataBegin = IR->GetData();

    auto Begin = IR->begin();
    auto Op = Begin();

    IR::OrderedNode *RealNode = Op->GetNode(ListBegin);
    auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
    LogMan::Throw::A(HeaderOp->Header.Op == IR::OP_IRHEADER, "First op wasn't IRHeader");

    IR::OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

    while (1) {
      auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
      LogMan::Throw::A(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

      BlockInterferences *BlockInterferenceVector = &LocalBlockInterferences.try_emplace(BlockNode->Wrapped(ListBegin).ID()).first->second;
      BlockInterferenceVector->reserve(BlockIROp->Last.ID() - BlockIROp->Begin.ID());

      // We grab these nodes this way so we can iterate easily
      auto CodeBegin = IR->at(BlockIROp->Begin);
      auto CodeLast = IR->at(BlockIROp->Last);
      while (1) {
        auto CodeOp = CodeBegin();
        uint32_t Node = CodeOp->ID();
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
    uintptr_t ListBegin = IR->GetListData();
    uintptr_t DataBegin = IR->GetData();

    auto Begin = IR->begin();
    auto Op = Begin();

    IR::OrderedNode *RealNode = Op->GetNode(ListBegin);
    auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
    LogMan::Throw::A(HeaderOp->Header.Op == IR::OP_IRHEADER, "First op wasn't IRHeader");

    IR::OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

    while (1) {
      auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
      LogMan::Throw::A(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

      BlockInterferences *BlockInterferenceVector = &LocalBlockInterferences.try_emplace(BlockNode->Wrapped(ListBegin).ID()).first->second;

      std::vector<uint32_t> Interferences;
      Interferences.reserve(BlockInterferenceVector->size() + GlobalBlockInterferences.size());

      // We grab these nodes this way so we can iterate easily
      auto CodeBegin = IR->at(BlockIROp->Begin);
      auto CodeLast = IR->at(BlockIROp->Last);
      while (1) {
        auto CodeOp = CodeBegin();
        uint32_t Node = CodeOp->ID();

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
      if (CurrentNode->Head.RegisterClass == INVALID_CLASS)
        continue;

      uint64_t Reg = ~0ULL;
      RegisterClass *RAClass = &Graph->Set.Classes[CurrentNode->Head.RegisterClass];
      if (CurrentNode->Head.PhiPartner) {
        // In the case that we have a list of nodes that need the same register allocated we need to do something special
        // We need to gather the data from the forward linked list and make sure they all match the virtual register
        std::vector<RegisterNode *> Nodes;
        auto CurrentPartner = CurrentNode;
        while (CurrentPartner) {
          Nodes.emplace_back(CurrentPartner);
          CurrentPartner = CurrentPartner->Head.PhiPartner;
        }

        for (uint32_t ri = 0; ri < RAClass->NumberOfRegisters; ++ri) {
          uint64_t RegisterToCheck = (static_cast<uint64_t>(CurrentNode->Head.RegisterClass) << 32) + ri;
          if (!DoesNodeSetInterfereWithRegister(Graph, Nodes, RegisterToCheck)) {
            Reg = RegisterToCheck;
            break;
          }
        }

        // If we failed to find a virtual register then allocate more space for them
        if (Reg == ~0ULL) {
          Reg = (static_cast<uint64_t>(CurrentNode->Head.RegisterClass) << 32);
          Reg |= AllocateMoreRegisters(Graph, CurrentNode->Head.RegisterClass);
        }

        TopRAPressure[CurrentNode->Head.RegisterClass] = std::max((uint32_t)Reg, TopRAPressure[CurrentNode->Head.RegisterClass]);

        // Walk the partners and ensure they are all set to the same register now
        for (auto Partner : Nodes) {
          Partner->Head.Register = Reg;
        }
      }
      else {
        for (uint32_t ri = 0; ri < RAClass->NumberOfRegisters; ++ri) {
          uint64_t RegisterToCheck = (static_cast<uint64_t>(CurrentNode->Head.RegisterClass) << 32) + ri;
          if (!DoesNodeInterfereWithRegister(Graph, CurrentNode, RegisterToCheck)) {
            Reg = RegisterToCheck;
            break;
          }
        }

        // If we failed to find a virtual register then allocate more space for them
        if (Reg == ~0ULL) {
          Reg = (static_cast<uint64_t>(CurrentNode->Head.RegisterClass) << 32);
          Reg |= AllocateMoreRegisters(Graph, CurrentNode->Head.RegisterClass);
        }

        TopRAPressure[CurrentNode->Head.RegisterClass] = std::max((uint32_t)Reg, TopRAPressure[CurrentNode->Head.RegisterClass]);
        CurrentNode->Head.Register = Reg;
      }
    }
  }

  FEXCore::IR::NodeWrapperIterator ConstrainedRAPass::FindFirstUse(FEXCore::IR::OpDispatchBuilder *Disp, FEXCore::IR::OrderedNode* Node, FEXCore::IR::NodeWrapperIterator Begin, FEXCore::IR::NodeWrapperIterator End) {
    auto CurrentIR = Disp->ViewIR();
    uintptr_t ListBegin = CurrentIR.GetListData();
    uintptr_t DataBegin = CurrentIR.GetData();

    uint32_t SearchID = Node->Wrapped(ListBegin).ID();

    while (1) {
      using namespace FEXCore::IR;
      OrderedNodeWrapper *WrapperOp = Begin();
      OrderedNode *RealNode = WrapperOp->GetNode(ListBegin);
      FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);

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

    return FEXCore::IR::NodeWrapperIterator::Invalid();
  }

  uint32_t ConstrainedRAPass::FindNodeToSpill(RegisterNode *RegisterNode, uint32_t CurrentLocation, LiveRange const *OpLiveRange) {
    uint32_t InterferenceToSpill = ~0U;
    uint32_t InterferenceLowestCost = ~0U;
    uint32_t InterferenceFarthest = 0;

    for (uint32_t j = 0; j < RegisterNode->Head.InterferenceCount; ++j) {
      uint32_t InterferenceNode = RegisterNode->InterferenceList[j];
      auto *InterferenceLiveRange = &LiveRanges[InterferenceNode];
      if (CurrentLocation < InterferenceLiveRange->Begin) {
        continue;
      }

      // If the interference's live range is past this op's live range then we can dump it
      if (InterferenceLiveRange->End > OpLiveRange->End &&
          InterferenceLiveRange->RematCost != 1) {

        bool Found = false;
        if (InterferenceLiveRange->End > InterferenceFarthest) {
          Found = true;
        }
        else if (InterferenceLiveRange->RematCost < InterferenceLowestCost) {
          Found = true;
        }

        if (Found) {
          InterferenceToSpill = j;
          InterferenceLowestCost = InterferenceLiveRange->RematCost;
          InterferenceFarthest = InterferenceLiveRange->End;
        }
      }
    }

    // Couldn't find register to spill
    // Be more aggressive
    if (InterferenceToSpill == ~0U) {
      for (uint32_t j = 0; j < RegisterNode->Head.InterferenceCount; ++j) {
        uint32_t InterferenceNode = RegisterNode->InterferenceList[j];
        auto *InterferenceLiveRange = &LiveRanges[InterferenceNode];

        if (CurrentLocation < InterferenceLiveRange->Begin) {
          continue;
        }

        if (InterferenceLiveRange->RematCost != 1) {
          bool Found = false;
          if (OpLiveRange->End != InterferenceLiveRange->End &&
              OpLiveRange->Begin > InterferenceLiveRange->Begin) {
            Found = true;
          }
          else if (OpLiveRange->End != InterferenceLiveRange->End) {
            Found = true;
          }

          if (Found) {
            InterferenceToSpill = j;
            InterferenceLowestCost = InterferenceLiveRange->RematCost;
            InterferenceFarthest = InterferenceLiveRange->End;
          }

        }
      }
    }

    if (InterferenceToSpill == ~0U) {
      LogMan::Msg::D("node %%ssa%d has %ld interferences, was dumped in to virtual reg %d", CurrentLocation, RegisterNode->Head.InterferenceCount, RegisterNode->Head.Register);
      for (uint32_t j = 0; j < RegisterNode->Head.InterferenceCount; ++j) {
        uint32_t InterferenceNode = RegisterNode->InterferenceList[j];
        auto *InterferenceLiveRange = &LiveRanges[InterferenceNode];

        LogMan::Msg::D("\tInt%d: Remat: %d [%d, %d)", j, InterferenceLiveRange->RematCost, InterferenceLiveRange->Begin, InterferenceLiveRange->End);
      }
    }
    LogMan::Throw::A(InterferenceToSpill != ~0U, "Couldn't find Node to spill");
    return RegisterNode->InterferenceList[InterferenceToSpill];
  }

  uint32_t ConstrainedRAPass::FindSpillSlot(uint32_t Node, uint32_t RegisterClass) {
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

  void ConstrainedRAPass::SpillRegisters(FEXCore::IR::OpDispatchBuilder *Disp) {
    using namespace FEXCore;

    auto IR = Disp->ViewIR();
    uintptr_t ListBegin = IR.GetListData();
    uintptr_t DataBegin = IR.GetData();

    auto Begin = IR.begin();
    auto Op = Begin();
    auto LastCursor = Disp->GetWriteCursor();

    IR::OrderedNode *RealNode = Op->GetNode(ListBegin);
    auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
    LogMan::Throw::A(HeaderOp->Header.Op == IR::OP_IRHEADER, "First op wasn't IRHeader");

    IR::OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

    while (1) {
      auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
      LogMan::Throw::A(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

      // We grab these nodes this way so we can iterate easily
      auto CodeBegin = IR.at(BlockIROp->Begin);
      auto CodeLast = IR.at(BlockIROp->Last);

      while (1) {
        auto CodeOp = CodeBegin();
        IR::OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
        auto IROp = CodeNode->Op(DataBegin);

        if (IROp->HasDest) {
          uint32_t Node = CodeOp->ID();
          RegisterNode *CurrentNode = &Graph->Nodes[Node];
          LiveRange *OpLiveRange = &LiveRanges[Node];

          // If this node is allocated above the number of physical registers we have then we need to search the interference list and spill the one
          // that is cheapest
          bool NeedsToSpill = CurrentNode->Head.Register >= PhysicalRegisterCount.at(CurrentNode->Head.RegisterClass);

          if (NeedsToSpill) {
            bool Spilled = false;

            // First let's just check for constants that we can just rematerialize instead of spilling
            for (uint32_t j = 0; j < CurrentNode->Head.InterferenceCount; ++j) {
              uint32_t InterferenceNode = CurrentNode->InterferenceList[j];
              if (LiveRanges[InterferenceNode].End > OpLiveRange->End &&
                  LiveRanges[InterferenceNode].RematCost == 1) { // CONSTANT
                // We want to end the live range of this value here and continue it on first use
                IR::OrderedNodeWrapper ConstantOp = IR::OrderedNodeWrapper::WrapOffset(InterferenceNode * sizeof(IR::OrderedNode));
                IR::OrderedNode *ConstantNode = ConstantOp.GetNode(ListBegin);
                FEXCore::IR::IROp_Constant const *ConstantIROp = ConstantNode->Op(DataBegin)->C<IR::IROp_Constant>();
                LogMan::Throw::A(ConstantIROp->Header.Op == IR::OP_CONSTANT, "This needs to be const");
                // First op post Spill
                auto NextIter = CodeBegin;
                auto FirstUseLocation = FindFirstUse(Disp, ConstantNode, NextIter, CodeLast);
                LogMan::Throw::A(FirstUseLocation != IR::NodeWrapperIterator::Invalid(), "At %%ssa%d Spilling Op %%ssa%d but Failure to find op use", CodeOp->ID(), InterferenceNode);
                if (FirstUseLocation != IR::NodeWrapperIterator::Invalid()) {
                  --FirstUseLocation;
                  IR::OrderedNodeWrapper *FirstUseOp = FirstUseLocation();
                  IR::OrderedNode *FirstUseOrderedNode = FirstUseOp->GetNode(ListBegin);
                  Disp->SetWriteCursor(FirstUseOrderedNode);
                  auto FilledConstant = Disp->_Constant(ConstantIROp->Constant);
                  Disp->ReplaceAllUsesWithInclusive(ConstantNode, FilledConstant, FirstUseLocation, CodeLast);
                  Spilled = true;
                }
                break;
              }
            }

            // If we didn't remat a constant then we need to do some real spilling
            if (!Spilled) {
              uint32_t InterferenceNode = FindNodeToSpill(CurrentNode, Node, OpLiveRange);
              if (InterferenceNode != ~0U) {
                uint32_t SpillSlot = FindSpillSlot(InterferenceNode, Graph->Nodes[InterferenceNode].Head.RegisterClass);
                RegisterNode *InterferenceRegisterNode = &Graph->Nodes[InterferenceNode];
                LogMan::Throw::A(SpillSlot != ~0U, "Interference Node doesn't have a spill slot!");
                LogMan::Throw::A(InterferenceRegisterNode->Head.Register != ~0U, "Interference node never assigned a register?");
                LogMan::Throw::A(InterferenceRegisterNode->Head.RegisterClass != ~0U, "Interference node never assigned a register class?");
                LogMan::Throw::A(InterferenceRegisterNode->Head.PhiPartner == nullptr, "We don't support spilling PHI nodes currently");

                // If the interference's live range is past this op's live range then we can dump it
                FEXCore::IR::OrderedNodeWrapper InterferenceOp = IR::OrderedNodeWrapper::WrapOffset(InterferenceNode * sizeof(IR::OrderedNode));
                FEXCore::IR::OrderedNode *InterferenceOrderedNode = InterferenceOp.GetNode(ListBegin);
                FEXCore::IR::IROp_Header *InterferenceIROp = InterferenceOrderedNode->Op(DataBegin);

                auto PrevIter = CodeBegin;
                --PrevIter;
                --PrevIter;
                Disp->SetWriteCursor(PrevIter()->GetNode(ListBegin));

                auto SpillOp = Disp->_SpillRegister(InterferenceOrderedNode, SpillSlot, {InterferenceRegisterNode->Head.RegisterClass});
                SpillOp.first->Header.Size = InterferenceIROp->Size;
                SpillOp.first->Header.Elements = InterferenceIROp->Elements;

                {
                  // First op post Spill
                  auto NextIter = CodeBegin;
                  ++NextIter;
                  auto FirstUseLocation = FindFirstUse(Disp, InterferenceOrderedNode, NextIter, CodeLast);

                  LogMan::Throw::A(FirstUseLocation != NodeWrapperIterator::Invalid(), "At %%ssa%d Spilling Op %%ssa%d but Failure to find op use", CodeOp->ID(), InterferenceNode);
                  if (FirstUseLocation != IR::NodeWrapperIterator::Invalid()) {
                    --FirstUseLocation;
                    IR::OrderedNodeWrapper *FirstUseOp = FirstUseLocation();
                    IR::OrderedNode *FirstUseOrderedNode = FirstUseOp->GetNode(ListBegin);

                    Disp->SetWriteCursor(FirstUseOrderedNode);

                    auto FilledInterference = Disp->_FillRegister(SpillSlot, {InterferenceRegisterNode->Head.RegisterClass});
                    FilledInterference.first->Header.Size = InterferenceIROp->Size;
                    FilledInterference.first->Header.Elements = InterferenceIROp->Elements;
                    Disp->ReplaceAllUsesWithInclusive(InterferenceOrderedNode, FilledInterference, FirstUseLocation, CodeLast);
                    Spilled = true;
                  }
                }
              }
            }

            Disp->SetWriteCursor(LastCursor);
            // We can't spill multiple times in a row. Need to restart
            if (Spilled) {
              return;
            }
          }
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

  bool ConstrainedRAPass::RunAllocateVirtualRegisters(FEXCore::IR::OpDispatchBuilder *Disp) {
    using namespace FEXCore;
    bool Changed = false;

    GlobalBlockInterferences.clear();
    LocalBlockInterferences.clear();

    TopRAPressure.assign(TopRAPressure.size(), 0);

    // We need to rerun compaction every step
    Changed |= LocalCompaction->Run(Disp);
    auto IR = Disp->ViewIR();

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


  bool ConstrainedRAPass::Run(OpDispatchBuilder *Disp) {
    bool Changed = false;

    SpillSlotCount = 0;
    Graph->SpillStack.clear();

    while (1) {
      HadFullRA = true;

      // Virtual allocation pass runs the compaction pass per run
      Changed |= RunAllocateVirtualRegisters(Disp);

      for (size_t i = 0; i < PhysicalRegisterCount.size(); ++i) {
        // Virtual registers fit completely within physical registers
        // Remap virtual 1:1 to physical
        HadFullRA &= TopRAPressure[i] < PhysicalRegisterCount[i];
      }

      if (HadFullRA) {
        break;
      }

      SpillRegisters(Disp);
      Changed = true;
    }

    return Changed;
  }

  FEXCore::IR::RegisterAllocationPass* CreateRegisterAllocationPass() {
    return new ConstrainedRAPass{};
  }
}
