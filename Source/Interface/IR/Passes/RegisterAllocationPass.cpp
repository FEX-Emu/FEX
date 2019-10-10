#include "Common/BitSet.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <iterator>

namespace FEXCore::IR {
  constexpr uint32_t INVALID_REG = ~0U;
  constexpr uint32_t INVALID_CLASS = ~0U;
  constexpr uint32_t DEFAULT_INTERFERENCE_LIST_SIZE = 128;

  struct Register {
  };

  struct RegisterClass {
    uint32_t RegisterBase;
    uint32_t NumberOfRegisters{0};
    BitSet<uint64_t> Registers;
  };

  struct RegisterAllocationPass::RegisterNode {
    uint32_t RegisterClass;
    uint32_t Register;
    uint32_t InterferenceCount;
    uint32_t InterferenceListSize;
    uint32_t *InterferenceList;
    BitSet<uint64_t> Interference;
  };

  static_assert(std::is_pod<RegisterAllocationPass::RegisterNode>::value, "We want this to be POD");

  struct RegisterAllocationPass::RegisterSet {
    Register *Registers;
    RegisterClass *RegisterClasses;
    uint32_t RegisterCount;
    uint32_t ClassCount;
  };

  struct SpillStackUnit {
    uint32_t Node;
    uint32_t Class;
    OrderedNode *SpilledNode;
  };

  struct RegisterAllocationPass::RegisterGraph {
    RegisterSet *Set;
    RegisterNode *Nodes;
    uint32_t NodeCount;
    uint32_t MaxNodeCount;
    std::vector<SpillStackUnit> SpillStack;
  };

  RegisterAllocationPass::RegisterSet *RegisterAllocationPass::AllocateRegisterSet(uint32_t RegisterCount, uint32_t ClassCount) {
    RegisterSet *Set = new RegisterSet;

    Set->RegisterCount = RegisterCount;
    Set->ClassCount = ClassCount;

    Set->Registers = static_cast<Register*>(calloc(RegisterCount, sizeof(Register)));
    Set->RegisterClasses = static_cast<RegisterClass*>(calloc(ClassCount, sizeof(RegisterClass)));

    for (uint32_t i = 0; i < ClassCount; ++i) {
      Set->RegisterClasses[i].Registers.Allocate(RegisterCount);
    }

    return Set;
  }

  void RegisterAllocationPass::FreeRegisterSet(RegisterAllocationPass::RegisterSet *Set) {
    for (uint32_t i = 0; i < Set->ClassCount; ++i) {
      Set->RegisterClasses[i].Registers.Free();
    }
    free(Set->RegisterClasses);
    free(Set->Registers);
    delete Set;
  }

  void RegisterAllocationPass::AddRegisters(RegisterAllocationPass::RegisterSet *Set, uint32_t Class, uint32_t RegistersBase, uint32_t RegisterCount) {
    for (uint32_t i = 0; i < RegisterCount; ++i) {
      Set->RegisterClasses[Class].Registers.Set(RegistersBase + i);
    }
    Set->RegisterClasses[Class].RegisterBase = RegistersBase;
    Set->RegisterClasses[Class].NumberOfRegisters += RegisterCount;
  }

  RegisterAllocationPass::RegisterGraph *RegisterAllocationPass::AllocateRegisterGraph(RegisterAllocationPass::RegisterSet *Set, uint32_t NodeCount) {
    Graph = new RegisterAllocationPass::RegisterGraph;
    Graph->Set = Set;
    Graph->NodeCount = NodeCount;
    Graph->MaxNodeCount = NodeCount;
    Graph->Nodes = static_cast<RegisterNode*>(calloc(NodeCount, sizeof(RegisterNode)));

    // Initialize nodes
    for (uint32_t i = 0; i < NodeCount; ++i) {
      Graph->Nodes[i].Register = INVALID_REG;
      Graph->Nodes[i].RegisterClass = INVALID_CLASS;
      Graph->Nodes[i].InterferenceListSize = DEFAULT_INTERFERENCE_LIST_SIZE;
      Graph->Nodes[i].InterferenceList = reinterpret_cast<uint32_t*>(calloc(Graph->Nodes[i].InterferenceListSize, sizeof(uint32_t)));
      Graph->Nodes[i].InterferenceCount = 0;
      Graph->Nodes[i].Interference.Allocate(NodeCount);
      Graph->Nodes[i].Interference.Clear(NodeCount);
    }

    return Graph;
  }

  void RegisterAllocationPass::ResetRegisterGraph(uint32_t NodeCount) {
    if (NodeCount > Graph->MaxNodeCount) {
      uint32_t OldNodeCount = Graph->MaxNodeCount;
      Graph->NodeCount = NodeCount;
      Graph->MaxNodeCount = NodeCount;
      Graph->Nodes = static_cast<RegisterNode*>(realloc(Graph->Nodes, NodeCount * sizeof(RegisterNode)));

      // Initialize nodes
      for (uint32_t i = 0; i < OldNodeCount; ++i) {
        Graph->Nodes[i].Register = INVALID_REG;
        Graph->Nodes[i].RegisterClass = INVALID_CLASS;
        Graph->Nodes[i].InterferenceCount = 0;
        Graph->Nodes[i].Interference.Realloc(NodeCount);
        Graph->Nodes[i].Interference.Clear(NodeCount);
      }

      for (uint32_t i = OldNodeCount; i < NodeCount; ++i) {
        Graph->Nodes[i].Register = INVALID_REG;
        Graph->Nodes[i].RegisterClass = INVALID_CLASS;
        Graph->Nodes[i].InterferenceListSize = DEFAULT_INTERFERENCE_LIST_SIZE;
        Graph->Nodes[i].InterferenceList = reinterpret_cast<uint32_t*>(calloc(Graph->Nodes[i].InterferenceListSize, sizeof(uint32_t)));
        Graph->Nodes[i].InterferenceCount = 0;
        Graph->Nodes[i].Interference.Allocate(NodeCount);
        Graph->Nodes[i].Interference.Clear(NodeCount);
      }
    }
    else {
      // We are only handling a node count of this size right now
      Graph->NodeCount = NodeCount;

      // Initialize nodes
      for (uint32_t i = 0; i < NodeCount; ++i) {
        Graph->Nodes[i].Register = INVALID_REG;
        Graph->Nodes[i].RegisterClass = INVALID_CLASS;
        Graph->Nodes[i].InterferenceCount = 0;
        Graph->Nodes[i].Interference.Clear(NodeCount);
      }
    }
  }

  void RegisterAllocationPass::FreeRegisterGraph() {
    for (uint32_t i = 0; i < Graph->MaxNodeCount; ++i) {
      RegisterNode *Node = &Graph->Nodes[i];
      Node->InterferenceCount = 0;
      Node->InterferenceListSize = 0;
      free(Node->InterferenceList);
      Node->Interference.Free();
    }

    free(Graph->Nodes);
    Graph->NodeCount = 0;
    Graph->MaxNodeCount = 0;
    delete Graph;
  }

  void RegisterAllocationPass::SetNodeClass(uint32_t Node, uint32_t Class) {
    Graph->Nodes[Node].RegisterClass = Class;
  }

  uint32_t RegisterAllocationPass::GetNodeRegister(uint32_t Node) {
    return Graph->Nodes[Node].Register;
  }

  static bool HasInterference(RegisterAllocationPass::RegisterGraph *Graph, RegisterAllocationPass::RegisterNode *Node, uint32_t Register) {
    for (uint32_t i = 0; i < Node->InterferenceCount; ++i) {
      RegisterAllocationPass::RegisterNode *IntNode = &Graph->Nodes[Node->InterferenceList[i]];
      if (IntNode->Register == Register) {
        return true;
      }
    }

    return false;
  }

  RegisterAllocationPass::RegisterNode *RegisterAllocationPass::GetRegisterNode(uint32_t Node) {
    return &Graph->Nodes[Node];
  }

  void RegisterAllocationPass::FindNodeClasses(IRListView<false> *CurrentIR) {
    uintptr_t ListBegin = CurrentIR->GetListData();
    uintptr_t DataBegin = CurrentIR->GetData();

    IR::NodeWrapperIterator Begin = CurrentIR->begin();
    IR::NodeWrapperIterator End = CurrentIR->end();

    while (Begin != End) {
      using namespace FEXCore::IR;

      OrderedNodeWrapper *WrapperOp = Begin();
      OrderedNode *RealNode = WrapperOp->GetNode(ListBegin);
      FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);

      if (IROp->HasDest) {
        // XXX: This needs to be better
        switch (IROp->Op) {
        case OP_LOADCONTEXT: {
          auto Op = IROp->C<IR::IROp_LoadContext>();
          if (Op->Size == 16)
            SetNodeClass(WrapperOp->ID(), FPRClass);
          else
            SetNodeClass(WrapperOp->ID(), GPRClass);
          break;
        }
        case OP_STORECONTEXT: {
          auto Op = IROp->C<IR::IROp_StoreContext>();
          if (Op->Size == 16)
            SetNodeClass(WrapperOp->ID(), FPRClass);
          else
            SetNodeClass(WrapperOp->ID(), GPRClass);
          break;
        }
        case IR::OP_LOADMEM: {
          auto Op = IROp->C<IR::IROp_LoadMem>();
          if (Op->Size == 16)
            SetNodeClass(WrapperOp->ID(), FPRClass);
          else
            SetNodeClass(WrapperOp->ID(), GPRClass);
          break;
        }
        case OP_ZEXT: {
          auto Op = IROp->C<IR::IROp_Zext>();
          LogMan::Throw::A(Op->SrcSize <= 64, "Can't support Zext of size: %ld", Op->SrcSize);

          if (Op->SrcSize == 64) {
            SetNodeClass(WrapperOp->ID(), FPRClass);
          }
          else {
            SetNodeClass(WrapperOp->ID(), GPRClass);
          }
          break;
        }
        case OP_CPUID: SetNodeClass(WrapperOp->ID(), FPRClass); break;
        default:
          if (IROp->Op >= IR::OP_VOR)
            SetNodeClass(WrapperOp->ID(), FPRClass);
          else
            SetNodeClass(WrapperOp->ID(), GPRClass);
          break;
        }
      }
      ++Begin;
    }
  }

  void RegisterAllocationPass::CalculateLiveRange(IRListView<false> *CurrentIR) {
    size_t Nodes = CurrentIR->GetSSACount();
    if (Nodes > LiveRanges.size()) {
      LiveRanges.resize(Nodes);
    }
    memset(&LiveRanges.at(0), 0xFF, Nodes * sizeof(LiveRange));

    uintptr_t ListBegin = CurrentIR->GetListData();
    uintptr_t DataBegin = CurrentIR->GetData();

    IR::NodeWrapperIterator Begin = CurrentIR->begin();
    IR::NodeWrapperIterator End = CurrentIR->end();

    constexpr uint32_t DEFAULT_REMAT_COST = 1000;
    while (Begin != End) {
      using namespace FEXCore::IR;
      OrderedNodeWrapper *WrapperOp = Begin();
      OrderedNode *RealNode = WrapperOp->GetNode(ListBegin);
      FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);
      uint32_t Node = WrapperOp->ID();

      // If the destination hasn't yet been set then set it now
      if (IROp->HasDest) {
        LogMan::Throw::A(LiveRanges[Node].Begin == ~0U, "Node begin already defined?");
        LiveRanges[Node].Begin = Node;
        // Default to ending right where it starts
        LiveRanges[Node].End = Node;
      }

      // Calculate remat cost
      switch (IROp->Op) {
      case OP_CONSTANT: LiveRanges[Node].RematCost = 1; break;
      case OP_LOADFLAG:
      case OP_LOADCONTEXT: LiveRanges[Node].RematCost = 10; break;
      case OP_LOADMEM: LiveRanges[Node].RematCost = 100; break;
      case OP_FILLREGISTER: LiveRanges[Node].RematCost = DEFAULT_REMAT_COST + 1; break;
      default: LiveRanges[Node].RematCost = DEFAULT_REMAT_COST; break;
      }

      for (uint8_t i = 0; i < IROp->NumArgs; ++i) {
        uint32_t ArgNode = IROp->Args[i].ID();
        // Set the node end to be at least here
        LiveRanges[ArgNode].End = Node;
      }

      ++Begin;
    }
  }

  void RegisterAllocationPass::CalculateNodeInterference(uint32_t NodeCount) {
    auto AddInterference = [&](uint32_t Node1, uint32_t Node2) {
      RegisterNode *Node = &Graph->Nodes[Node1];
      Node->Interference.Set(Node2);
      if (Node->InterferenceListSize <= Node->InterferenceCount) {
        Node->InterferenceListSize *= 2;
        Node->InterferenceList = reinterpret_cast<uint32_t*>(realloc(Node->InterferenceList, Node->InterferenceListSize * sizeof(uint32_t)));
      }
      Node->InterferenceList[Node->InterferenceCount] = Node2;
      ++Node->InterferenceCount;
    };

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

  void RegisterAllocationPass::AllocateRegisters() {
    Graph->SpillStack.clear();
    for (uint32_t i = 0; i < Graph->NodeCount; ++i) {
      RegisterNode *CurrentNode = &Graph->Nodes[i];
      if (CurrentNode->RegisterClass == INVALID_CLASS)
        continue;

      uint32_t Reg = ~0U;
      RegisterClass *RAClass = &Graph->Set->RegisterClasses[CurrentNode->RegisterClass];
      for (uint32_t ri = 0; ri < RAClass->NumberOfRegisters; ++ri) {
        if (!HasInterference(Graph, CurrentNode, RAClass->RegisterBase + ri)) {
          Reg = ri;
          break;
        }
      }

      if (Reg == ~0U) {
        auto RegisterNode = GetRegisterNode(i);
        // LogMan::Msg::E("\t%%ssa%d with no-RA has live range [%d, %d): Remat cost: %d", i, LiveRanges[i].Begin, LiveRanges[i].End, LiveRanges[i].RematCost);
        for (uint32_t j = 0; j < RegisterNode->InterferenceCount; ++j) {
          uint32_t InterferenceNode = RegisterNode->InterferenceList[j];
          // LogMan::Msg::E("\t\tInterferes with %%ssa%d: live range[%d, %d): Remat cost: %d", InterferenceNode, LiveRanges[InterferenceNode].Begin, LiveRanges[InterferenceNode].End, LiveRanges[InterferenceNode].RematCost);
        }
        Graph->SpillStack.emplace_back(SpillStackUnit{i, CurrentNode->RegisterClass});
      }
      else {
        CurrentNode->Register = RAClass->RegisterBase + Reg;
      }
    }

    if (!Graph->SpillStack.empty()) {
      LogMan::Msg::E("Couldn't allocate %ld registers\n", Graph->SpillStack.size());
      HasSpills = true;
    }
  }

  static std::vector<SpillStackUnit>::iterator IsInSpillStack(std::vector<SpillStackUnit> *Stack, uint32_t ID) {
    for (auto Unit = Stack->begin(); Unit < Stack->end(); ++Unit) {
      if (Unit->Node == ID) return Unit;
    }

    return Stack->end();
  }

  IR::NodeWrapperIterator RegisterAllocationPass::FindFirstUse(OpDispatchBuilder *Disp, OrderedNode* Node, IR::NodeWrapperIterator Begin, IR::NodeWrapperIterator End) {
    auto CurrentIR = Disp->ViewIR();
    uintptr_t ListBegin = CurrentIR.GetListData();
    uintptr_t DataBegin = CurrentIR.GetData();

    while (Begin != End) {
      using namespace FEXCore::IR;
      OrderedNodeWrapper *WrapperOp = Begin();
      OrderedNode *RealNode = WrapperOp->GetNode(ListBegin);
      FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);

      LogMan::Msg::D("\t\t%%ssa%d Do we have %%ssa%d", WrapperOp->ID(), Node->Wrapped(ListBegin).ID());
      for (uint8_t i = 0; i < IROp->NumArgs; ++i) {
        uint32_t ArgNode = IROp->Args[i].ID();
        LogMan::Msg::D("\t\t\tArg[%d] %%ssa%d == %%ssa%d ?", i, ArgNode, Node->Wrapped(ListBegin).ID());
        if (ArgNode == Node->Wrapped(ListBegin).ID()) {
          LogMan::Msg::D("\t\t\tFound it");
          return Begin;
        }
      }

      ++Begin;
    }

    return End;
  }

  void RegisterAllocationPass::ClearSpillList(OpDispatchBuilder *Disp) {
    auto CurrentIR = Disp->ViewIR();
    uintptr_t ListBegin = CurrentIR.GetListData();
    uintptr_t DataBegin = CurrentIR.GetData();

    IR::NodeWrapperIterator Begin = CurrentIR.begin();
    IR::NodeWrapperIterator End = CurrentIR.end();

    auto LastCursor = Disp->GetWriteCursor();

    while (Begin != End) {
      OrderedNodeWrapper *WrapperOp = Begin();
      OrderedNode *RealNode = WrapperOp->GetNode(ListBegin);
      FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);
      auto Iter = IsInSpillStack(&Graph->SpillStack, WrapperOp->ID());
      if (Iter != Graph->SpillStack.end()) {
        auto RegisterNode = GetRegisterNode(Iter->Node);
        auto *OpLiveRange = &LiveRanges[Iter->Node];
        auto SpillSlot = std::distance(Graph->SpillStack.begin(), Iter);

        bool Spilled = false;
        if (1) {
          for (uint32_t j = 0; j < RegisterNode->InterferenceCount; ++j) {
            uint32_t InterferenceNode = RegisterNode->InterferenceList[j];
            if (LiveRanges[InterferenceNode].End > OpLiveRange->End &&
                LiveRanges[InterferenceNode].RematCost == 1) { // CONSTANT
              // We want to end the live range of this value here and continue it on first use
              auto ConstantRegisterNode = GetRegisterNode(InterferenceNode);
              auto *ConstantLiveRange = &LiveRanges[InterferenceNode];
              OrderedNodeWrapper ConstantOp = OrderedNodeWrapper::WrapOffset(InterferenceNode * sizeof(OrderedNode));
              OrderedNode *ConstantNode = ConstantOp.GetNode(ListBegin);
              FEXCore::IR::IROp_Constant const *ConstantIROp = ConstantNode->Op(DataBegin)->C<IR::IROp_Constant>();
              LogMan::Throw::A(ConstantIROp->Header.Op == OP_CONSTANT, "This needs to be const");

              // First op post Spill
              auto NextIter = Begin;
              ++NextIter;
              auto FirstUseLocation = FindFirstUse(Disp, ConstantNode, NextIter, End);
              if (FirstUseLocation != End) {
                // LogMan::Throw::A(FirstUseLocation != End, "Failure to find op use");
                OrderedNodeWrapper *FirstUseOp = FirstUseLocation();
                OrderedNode *FirstUseOrderedNode = FirstUseOp->GetNode(ListBegin);
                Disp->SetWriteCursor(FirstUseOrderedNode);
                auto FilledConstant = Disp->_Constant(ConstantIROp->Constant);
                Disp->ReplaceAllUsesWithInclusive(ConstantNode, FilledConstant, FirstUseLocation, End);
              }
              Spilled = true;
              break;
            }
          }
        }

        if (!Spilled) {
          uint32_t InterferenceToSpill = ~0U;
          uint32_t InterferenceLowestCost = ~0U;
          uint32_t InterferenceFarthest = 0;

          for (uint32_t j = 0; j < RegisterNode->InterferenceCount; ++j) {
            uint32_t InterferenceNode = RegisterNode->InterferenceList[j];
            auto *InterferenceLiveRange = &LiveRanges[InterferenceNode];
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
            for (uint32_t j = 0; j < RegisterNode->InterferenceCount; ++j) {
              uint32_t InterferenceNode = RegisterNode->InterferenceList[j];
              auto *InterferenceLiveRange = &LiveRanges[InterferenceNode];
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

          if (InterferenceToSpill != ~0U) {
            uint32_t InterferenceNode = RegisterNode->InterferenceList[InterferenceToSpill];
            auto InterferenceRegisterNode = GetRegisterNode(InterferenceNode);
            auto *InterferenceLiveRange = &LiveRanges[InterferenceNode];
            // If the interference's live range is past this op's live range then we can dump it
            if (1) {
              OrderedNodeWrapper InterferenceOp = OrderedNodeWrapper::WrapOffset(InterferenceNode * sizeof(OrderedNode));
              OrderedNode *InterferenceOrderedNode = InterferenceOp.GetNode(ListBegin);
              FEXCore::IR::IROp_Header *InterferenceIROp = InterferenceOrderedNode->Op(DataBegin);

              auto PrevIter = Begin;
              --PrevIter;
              Disp->SetWriteCursor(PrevIter()->GetNode(ListBegin));
              auto SpillOp = Disp->_SpillRegister(InterferenceOrderedNode, SpillSlotCount, {InterferenceRegisterNode->RegisterClass});
              SpillOp.first->Header.Size = InterferenceIROp->Size;
              SpillOp.first->Header.Elements = InterferenceIROp->Elements;

              {
                // First op post Spill
                auto NextIter = Begin;
                ++NextIter;
                auto FirstUseLocation = FindFirstUse(Disp, InterferenceOrderedNode, NextIter, End);
                if (FirstUseLocation != End) {
                  // LogMan::Throw::A(FirstUseLocation != End, "Failure to find op use");
                  OrderedNodeWrapper *FirstUseOp = FirstUseLocation();
                  OrderedNode *FirstUseOrderedNode = FirstUseOp->GetNode(ListBegin);
                  Disp->SetWriteCursor(FirstUseOrderedNode);

                  auto FilledInterference = Disp->_FillRegister(SpillSlotCount, {InterferenceRegisterNode->RegisterClass});
                  LogMan::Msg::D("\t\tFill at %%ssa%d", FilledInterference.Node->Wrapped(ListBegin).ID());
                  FilledInterference.first->Header.Size = InterferenceIROp->Size;
                  FilledInterference.first->Header.Elements = InterferenceIROp->Elements;
                  Disp->ReplaceAllUsesWithInclusive(InterferenceOrderedNode, FilledInterference, FirstUseLocation, End);
                }
                Spilled = true;
              }

              ++SpillSlotCount;
              break;
            }
          }
        }

        // LogMan::Throw::A(SpillSlotCount <= Graph->SpillStack.size(), "Managed to hit more spill locations than in the spill stack. %d <= %ld", SpillSlotCount, Graph->SpillStack.size());
        // auto ConstantVal = Disp->_Constant(NewConstant);
        // Disp->ReplaceAllUsesWith(RealNode, ConstantVal);
        LogMan::Throw::A(Spilled, "We should have spilled by now");

        Disp->SetWriteCursor(LastCursor);
        Graph->SpillStack.erase(Iter);
      }

      ++Begin;
    }
  }

  bool RegisterAllocationPass::Run(OpDispatchBuilder *Disp) {
    bool Changed = false;
    constexpr uint32_t RATries = 1;

    SpillSlotCount = 0;
    HasSpills = false;
    HadFullRA = false;
    for (uint32_t i = 0; i < RATries; ++i) {
      auto CurrentIR = Disp->ViewIR();

      uintptr_t ListSize = CurrentIR.GetListSize();
      uint32_t SSACount = CurrentIR.GetSSACount();

      ResetRegisterGraph(SSACount);
      FindNodeClasses(&CurrentIR);

      CalculateLiveRange(&CurrentIR);
      CalculateNodeInterference(SSACount);
      AllocateRegisters();
      if (!Graph->SpillStack.empty()) {
        if (Config_SupportsSpills) {
          Disp->ShouldDump = true;
          Changed = true;
          ClearSpillList(Disp);
        }
        else {
          HadFullRA = false;
          SpillSlotCount = 0;
        }
        return Changed;
      }
      else {
        // We managed to RA, leave now
        HadFullRA = true;
        return Changed;
      }
    }

    return Changed;
  }

  RegisterAllocationPass::RegisterAllocationPass() {
    LiveRanges.resize(9000);
  }
}

