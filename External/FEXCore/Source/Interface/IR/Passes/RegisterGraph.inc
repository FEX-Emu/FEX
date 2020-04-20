#include "Common/BitSet.h"
#include "Interface/IR/Passes.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <iterator>

#include <stdint.h>

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
      RegisterNode *TiePartner;
      uint32_t TempsBase;
      uint32_t PhysicalsBase;
      bool PinnedReg;
    } Head;

    uint32_t InterferenceListSize;
    uint32_t *InterferenceList;
    BitSetView<uint64_t> Interference;
  };

  constexpr RegisterNode::VolatileHeader DefaultNodeHeader = {
    .RegAndClass = INVALID_REGCLASS,
    .InterferenceCount = 0,
    .BlockID = ~0U,
    .SpillSlot = ~0U,
    .TiePartner = nullptr,
    .TempsBase = ~0U,
    .PhysicalsBase = ~0U,
    .PinnedReg = false,
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
    LogMan::Msg::D("Allocated up to %d virtual registers\n", LocalClass.Count);
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

  uint32_t AddTempNode(RegisterGraph *Graph) {
    uint32_t OldNodeCount = Graph->NodeCount;
    uint32_t NewNodeCount = OldNodeCount + 1;
    if (NewNodeCount > Graph->MaxNodeCount) {
      // We need to allocate more nodes for this temp
      uint32_t NewNodeCount = OldNodeCount * 2;
      Graph->MaxNodeCount = NewNodeCount;
      Graph->Nodes = static_cast<RegisterNode*>(realloc(Graph->Nodes, NewNodeCount * sizeof(RegisterNode)));

      // Initialize all these new nodes
      for (uint32_t i = OldNodeCount; i < NewNodeCount; ++i) {
        Graph->Nodes[i].Head = DefaultNodeHeader;
        Graph->Nodes[i].InterferenceListSize = DEFAULT_INTERFERENCE_LIST_COUNT;
        Graph->Nodes[i].InterferenceList = reinterpret_cast<uint32_t*>(calloc(Graph->Nodes[i].InterferenceListSize, sizeof(uint32_t)));
        Graph->Nodes[i].Interference.GetView(Graph->InterferenceSet, NewNodeCount * i);
      }
    }

    // We can just increase the node count here
    ++Graph->NodeCount;

    return OldNodeCount;
  }

  void SetNodeClass(RegisterGraph *Graph, uint32_t Node, FEXCore::IR::RegisterClassType Class) {
    Graph->Nodes[Node].Head.RegAndClass = ((uint64_t)Class << 32) | (Graph->Nodes[Node].Head.RegAndClass & ~0U);
  }

  void SetNodePartner(RegisterGraph *Graph, uint32_t Node, uint32_t Partner) {
    Graph->Nodes[Node].Head.TiePartner = &Graph->Nodes[Partner];
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

  bool IsPinnedRegisterFreeAtNode(RegisterGraph *Graph, RegisterNode const *Node, uint64_t RegAndClass) {
    for (uint32_t i = 0; i < Node->Head.InterferenceCount; ++i) {
      RegisterNode *InterferenceNode = &Graph->Nodes[Node->InterferenceList[i]];
      if (InterferenceNode->Head.RegAndClass == RegAndClass &&
          InterferenceNode != Node) {
        return false;
      }

      FEXCore::IR::RegisterClassType InterferenceClass = FEXCore::IR::RegisterClassType{(uint32_t)(InterferenceNode->Head.RegAndClass >> 32)};
      uint32_t InterferenceReg = InterferenceNode->Head.RegAndClass;
      if (InterferenceReg != INVALID_REG &&
          !Graph->Set.Classes[InterferenceClass].Conflicts.empty() &&
          Graph->Set.Classes[InterferenceClass].Conflicts[InterferenceReg] == RegAndClass) {
        return false;
      }
    }

    return true;
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

  FEXCore::IR::RegisterClassType GetRegClassFromNode(uintptr_t ListBegin, uintptr_t DataBegin, FEXCore::IR::OrderedNodeWrapper const WrapperOp) {
    using namespace FEXCore;
    IR::OrderedNode const *RealNode = WrapperOp.GetNode(ListBegin);
    IR::IROp_Header const *IROp = RealNode->Op(DataBegin);

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
      case IR::OP_LOADMEM: {
        auto Op = IROp->C<IR::IROp_LoadMem>();
        return Op->Class;
        break;
      }
      case IR::OP_FILLREGISTER: {
        auto Op = IROp->C<IR::IROp_FillRegister>();
        return Op->Class;
        break;
      }
      case IR::OP_ZEXT: {
        auto Op = IROp->C<IR::IROp_Zext>();
        LogMan::Throw::A(Op->SrcSize <= 64, "Can't support Zext of size: %ld", Op->SrcSize);

        if (Op->SrcSize == 64) {
          return FEXCore::IR::FPRClass;
        }
        else {
          return FEXCore::IR::GPRClass;
        }
        break;
      }
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
      default: break;
    }

    // Unreachable
    return FEXCore::IR::InvalidClass;
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

  auto AddInterference = [](RegisterGraph *Graph, uint32_t Node1, uint32_t Node2) {
    RegisterNode *Node = &Graph->Nodes[Node1];
    Node->Interference.Set(Node2);
    Node->InterferenceList[Node->Head.InterferenceCount++] = Node2;
    if (Node->InterferenceListSize <= Node->Head.InterferenceCount) {
      Node->InterferenceListSize *= 2;
      Node->InterferenceList = reinterpret_cast<uint32_t*>(realloc(Node->InterferenceList, Node->InterferenceListSize * sizeof(uint32_t)));
    }
  };

  void CalculateNodeInterference(RegisterGraph *Graph, FEXCore::IR::IRListView<false> *IR, std::vector<LiveRange> *LiveRanges) {
    uint32_t NodeCount = Graph->NodeCount;

    // Now that we have all the live ranges calculated we need to add them to our interference graph
    for (uint32_t i = 0; i < NodeCount; ++i) {
      for (uint32_t j = i + 1; j < NodeCount; ++j) {
        if (!(LiveRanges->at(i).Begin >= LiveRanges->at(j).End ||
              LiveRanges->at(j).Begin >= LiveRanges->at(i).End)) {
          AddInterference(Graph, i, j);
          AddInterference(Graph, j, i);
        }
      }
    }
  }

  void CalculateLiveRange(RegisterGraph *Graph, FEXCore::IR::IRListView<false> *IR, std::vector<LiveRange> *LiveRanges) {
    using namespace FEXCore;
    size_t Nodes = IR->GetSSACount();
    if (Nodes > LiveRanges->size()) {
      LiveRanges->resize(Nodes);
    }
    LiveRanges->assign(Nodes * sizeof(LiveRange), {~0U, ~0U});

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
          LogMan::Throw::A(LiveRanges->at(Node).Begin == ~0U, "Node begin already defined?");
          LiveRanges->at(Node).Begin = Node;
          // Default to ending right where it starts
          LiveRanges->at(Node).End = Node;
        }

        // Calculate remat cost
        switch (IROp->Op) {
          case IR::OP_CONSTANT: LiveRanges->at(Node).RematCost = 1; break;
          case IR::OP_LOADFLAG:
          case IR::OP_LOADCONTEXT: LiveRanges->at(Node).RematCost = 10; break;
          case IR::OP_LOADMEM: LiveRanges->at(Node).RematCost = 100; break;
          case IR::OP_FILLREGISTER: LiveRanges->at(Node).RematCost = DEFAULT_REMAT_COST + 1; break;
          // We want PHI to be very expensive to spill
          case IR::OP_PHI: LiveRanges->at(Node).RematCost = DEFAULT_REMAT_COST * 10; break;
          default: LiveRanges->at(Node).RematCost = DEFAULT_REMAT_COST; break;
        }

        uint8_t NumArgs = IR::GetArgs(IROp->Op);
        for (uint8_t i = 0; i < NumArgs; ++i) {
          if (IROp->Args[i].IsInvalid()) continue;
          uint32_t ArgNode = IROp->Args[i].ID();
          // Set the node end to be at least here
          LiveRanges->at(ArgNode).End = Node;
          LogMan::Throw::A(LiveRanges->at(ArgNode).Begin != ~0U, "%%ssa%d used by %%ssa%d before defined?", ArgNode, Node);
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

        FEXCore::IR::Arch::OpConstraints Constraint = FEXCore::IR::Arch::GetOpConstraints(IROp->Op);
        auto &GraphNode = Graph->Nodes[Node];

        if (Constraint.Flags & FEXCore::IR::Constraint_Src0_Is_LateKill) {
          uint32_t ArgNode = IROp->Args[0].ID();
          // Live range is +1
          LiveRanges->at(ArgNode).End++;
        }

        if (Constraint.Flags & FEXCore::IR::Constraint_Src1_Is_LateKill) {
          uint32_t ArgNode = IROp->Args[1].ID();
          // Live range is +1
          LiveRanges->at(ArgNode).End++;
        }

        if (Constraint.Flags & FEXCore::IR::Constraint_Src2_Is_LateKill) {
          uint32_t ArgNode = IROp->Args[2].ID();
          // Live range is +1
          LiveRanges->at(ArgNode).End++;
        }

        if (Constraint.Flags & FEXCore::IR::Constraint_Src3_Is_LateKill) {
          uint32_t ArgNode = IROp->Args[3].ID();
          // Live range is +1
          LiveRanges->at(ArgNode).End++;
        }

        if (Constraint.Flags & FEXCore::IR::Constraint_Needs_Temps) {
          uint32_t MoreTemps = Constraint.NumTempsGPR + Constraint.NumTempsFPR + Constraint.NumTempsGPRPair;
          uint32_t OldGraphSize = Graph->NodeCount;
          uint32_t NewGraphSize = OldGraphSize + MoreTemps;
          if (NewGraphSize > LiveRanges->size()) {
            size_t OldSize = LiveRanges->size();
            LiveRanges->resize(OldSize + MoreTemps);
          }

          auto AddTmpWithClass = [&](FEXCore::IR::RegisterClassType Class) {
            uint32_t NewTempNode = AddTempNode(Graph);
            auto Range = &LiveRanges->at(NewTempNode);
            Range->Begin = Node;
            Range->End = Node; // Technically it is considered late-kill so +1

            auto &TempNode = Graph->Nodes[NewTempNode];
            // Convert the reg classification over to our virtual reg size
            TempNode.Head.RegAndClass = (((uint64_t)Class << 32) | INVALID_REG);
            LogMan::Msg::D("[Temp] Node %d has class %d and No allocated reg", NewTempNode, Class.Val);

            return NewTempNode;
          };

          for (uint32_t i = 0; i < Constraint.NumTempsGPR; ++i) {
            uint32_t NewNode = AddTmpWithClass(FEXCore::IR::GPRClass);
            if (i == 0) {
              GraphNode.Head.TempsBase = NewNode;
            }
          }

          for (uint32_t i = 0; i < Constraint.NumTempsFPR; ++i) {
            AddTmpWithClass(FEXCore::IR::FPRClass);
          }

          for (uint32_t i = 0; i < Constraint.NumTempsGPRPair; ++i) {
            AddTmpWithClass(FEXCore::IR::GPRPairClass);
          }
        }

        if (Constraint.Flags & FEXCore::IR::Constraint_Dest_Is_Src0) {
          SetNodePartner(Graph, Node, IROp->Args[0].ID());
        }

        if (Constraint.Flags & FEXCore::IR::Constraint_Dest_Is_Physical) {
          // 0 is dest, rest are source
          GraphNode.Head.PinnedReg = true;
          GraphNode.Head.RegAndClass = ((uint64_t)(Constraint.SrcPhysicalAssignment[0] >> 6) << 32) | (Constraint.SrcPhysicalAssignment[0] & 0b11111);
        }

        if (Constraint.Flags & FEXCore::IR::Constraint_Src0_Is_Physical) {
          auto &SrcNode = Graph->Nodes[IROp->Args[0].ID()];
          SrcNode.Head.PinnedReg = true;
          SrcNode.Head.RegAndClass = ((uint64_t)(Constraint.SrcPhysicalAssignment[1] >> 6) << 32) | (Constraint.SrcPhysicalAssignment[1] & 0b11111);
        }

        if (Constraint.Flags & FEXCore::IR::Constraint_Src1_Is_Physical) {
          auto &SrcNode = Graph->Nodes[IROp->Args[1].ID()];
          SrcNode.Head.PinnedReg = true;
          SrcNode.Head.RegAndClass = ((uint64_t)(Constraint.SrcPhysicalAssignment[2] >> 6) << 32) | (Constraint.SrcPhysicalAssignment[2] & 0b11111);
        }

        if (Constraint.Flags & FEXCore::IR::Constraint_Src2_Is_Physical) {
          auto &SrcNode = Graph->Nodes[IROp->Args[2].ID()];
          SrcNode.Head.PinnedReg = true;
          SrcNode.Head.RegAndClass = ((uint64_t)(Constraint.SrcPhysicalAssignment[3] >> 6) << 32) | (Constraint.SrcPhysicalAssignment[3] & 0b11111);
        }

        if (Constraint.Flags & FEXCore::IR::Constraint_Src3_Is_Physical) {
          auto &SrcNode = Graph->Nodes[IROp->Args[3].ID()];
          SrcNode.Head.PinnedReg = true;
          SrcNode.Head.RegAndClass = ((uint64_t)(Constraint.SrcPhysicalAssignment[4] >> 6) << 32) | (Constraint.SrcPhysicalAssignment[4] & 0b11111);
        }

        if (Constraint.Flags & FEXCore::IR::Constraint_Needs_Physicals) {
          for (uint32_t i = 0; i < (sizeof(Constraint.NeedsPhysicalRegisters) / sizeof(Constraint.NeedsPhysicalRegisters[0])); ++i) {
            // Invalid then leave
            if (Constraint.NeedsPhysicalRegisters[i] == 255) break;

            uint32_t OldGraphSize = Graph->NodeCount;
            uint32_t NewGraphSize = OldGraphSize + 1;
            if (NewGraphSize > LiveRanges->size()) {
              size_t OldSize = LiveRanges->size();
              LiveRanges->resize(OldSize + 1);
            }

            uint32_t NewTempNode = AddTempNode(Graph);

            if (i == 0) {
              GraphNode.Head.PhysicalsBase = NewTempNode;
            }

            auto Range = &LiveRanges->at(NewTempNode);
            Range->Begin = Node - 1; // This is early-clobber and late kill
            Range->End = Node; // Technically it is considered late-kill so +1

            auto &TempNode = Graph->Nodes[NewTempNode];
            TempNode.Head.PinnedReg = true;
            LogMan::Msg::D("[Pinned] Node %d has class %d and reg %d", NewTempNode, Constraint.NeedsPhysicalRegisters[i] >> 6, Constraint.NeedsPhysicalRegisters[i] & 0b11111);

            // Convert the reg classification over to our virtual reg size
            TempNode.Head.RegAndClass = ((uint64_t)(Constraint.NeedsPhysicalRegisters[i] >> 6) << 32) | (Constraint.NeedsPhysicalRegisters[i] & 0b11111);
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

}
