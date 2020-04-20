#include "Common/BitSet.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/OpcodeDispatcher.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"
#include "Interface/IR/Passes.h"

#include "Interface/IR/Passes/RegisterGraph.inc"

namespace {
  class ConstraintRAPass final : public FEXCore::IR::RegisterAllocationPass {
    public:
      ConstraintRAPass();
      ~ConstraintRAPass();
      bool Run(FEXCore::IR::OpDispatchBuilder *Disp) override;

      void AllocateRegisterSet(uint32_t RegisterCount, uint32_t ClassCount) override;
      void AddRegisters(FEXCore::IR::RegisterClassType Class, uint32_t RegisterCount) override;
      void AddRegisterConflict(FEXCore::IR::RegisterClassType ClassConflict, uint32_t RegConflict, FEXCore::IR::RegisterClassType Class, uint32_t Reg) override;
      void AllocateRegisterConflicts(FEXCore::IR::RegisterClassType Class, uint32_t NumConflicts) override;

      /**
       * @brief Returns the register and class encoded together
       * Top 32bits is the class, lower 32bits is the register
       */
      uint64_t GetDestRegister(uint32_t Node) override;
      uint64_t GetTemp(uint32_t Node, uint8_t Index) override;
      uint64_t GetPhysicalTemp(uint32_t Node, uint8_t Index) override;

    private:
      std::vector<uint32_t> PhysicalRegisterCount;
      std::vector<uint32_t> TopRAPressure;

      std::vector<LiveRange> LiveRanges;

      RegisterGraph *Graph;
      std::unique_ptr<FEXCore::IR::Pass> LocalCompaction;

      void AllocateVirtualRegisters(FEXCore::IR::IRListView<false> *IR);
  };

  ConstraintRAPass::ConstraintRAPass() {
    LocalCompaction.reset(FEXCore::IR::CreateIRCompaction());
  }

  ConstraintRAPass::~ConstraintRAPass() {
    FreeRegisterGraph(Graph);
  }

  void ConstraintRAPass::AllocateRegisterSet(uint32_t RegisterCount, uint32_t ClassCount) {
    // We don't care about Max register count
    PhysicalRegisterCount.resize(ClassCount);
    TopRAPressure.resize(ClassCount);

    Graph = AllocateRegisterGraph(ClassCount);
  }

  void ConstraintRAPass::AddRegisters(FEXCore::IR::RegisterClassType Class, uint32_t RegisterCount) {
    AllocateRegisters(Graph, Class, DEFAULT_VIRTUAL_REG_COUNT);
    AllocatePhysicalRegisters(Graph, Class, RegisterCount);
    PhysicalRegisterCount[Class] = RegisterCount;
  }

  void ConstraintRAPass::AddRegisterConflict(FEXCore::IR::RegisterClassType ClassConflict, uint32_t RegConflict, FEXCore::IR::RegisterClassType Class, uint32_t Reg) {
    VirtualAddRegisterConflict(Graph, ClassConflict, RegConflict, Class, Reg);
  }

  void ConstraintRAPass::AllocateRegisterConflicts(FEXCore::IR::RegisterClassType Class, uint32_t NumConflicts) {
    VirtualAllocateRegisterConflicts(Graph, Class, NumConflicts);
  }

  uint64_t ConstraintRAPass::GetDestRegister(uint32_t Node) {
    return Graph->Nodes[Node].Head.RegAndClass;
  }

  uint64_t ConstraintRAPass::GetTemp(uint32_t Node, uint8_t Index) {
    auto GraphNode = &Graph->Nodes[Node];
    return Graph->Nodes[GraphNode->Head.TempsBase + Index].Head.RegAndClass;
  }

  uint64_t ConstraintRAPass::GetPhysicalTemp(uint32_t Node, uint8_t Index) {
    auto GraphNode = &Graph->Nodes[Node];
    return Graph->Nodes[GraphNode->Head.PhysicalsBase + Index].Head.RegAndClass;
  }

  void ConstraintRAPass::AllocateVirtualRegisters(FEXCore::IR::IRListView<false> *IR) {
    uintptr_t ListBegin = IR->GetListData();
    uintptr_t DataBegin = IR->GetData();


    auto FindPinnedRegForClass = [&](RegisterNode *CurrentNode, uint64_t RegAndClass) {
      uint32_t Class = RegAndClass >> 32;
      if (IsPinnedRegisterFreeAtNode(Graph, CurrentNode, RegAndClass)) {
        TopRAPressure[Class] = std::max((uint32_t)RegAndClass, TopRAPressure[Class]);
      }
      else {
        LogMan::Msg::A("Couldn't fulfill physical register constraint 0x%lx", RegAndClass);
      }
    };

    auto FindRegForClass = [&](FEXCore::IR::RegisterClassType Class, RegisterNode *CurrentNode) {
      LogMan::Throw::A(Class <= FEXCore::IR::GPRPairClass.Val, "Unsupported RegClass %d", Class.Val);
      uint64_t RegAndClass = ~0ULL;
      RegisterClass *RAClass = &Graph->Set.Classes[Class.Val];
      for (uint32_t ri = 0; ri < RAClass->Count; ++ri) {
        uint64_t RegisterToCheck = (static_cast<uint64_t>(Class) << 32) + ri;
        if (!DoesNodeInterfereWithRegister(Graph, CurrentNode, RegisterToCheck)) {
          RegAndClass = RegisterToCheck;
          break;
        }
      }

      // If we failed to find a virtual register then allocate more space for them
      if (RegAndClass == ~0ULL) {
        RegAndClass = (static_cast<uint64_t>(Class) << 32);
        RegAndClass |= AllocateMoreRegisters(Graph, Class);
      }

      TopRAPressure[Class] = std::max((uint32_t)RegAndClass, TopRAPressure[Class]);
      CurrentNode->Head.RegAndClass = RegAndClass;
    };

    auto FindRegForNodeSet = [&](FEXCore::IR::RegisterClassType Class, std::vector<RegisterNode *> &Nodes) {
      uint64_t RegAndClass = ~0ULL;
      RegisterClass *RAClass = &Graph->Set.Classes[Class.Val];
      for (uint32_t ri = 0; ri < RAClass->Count; ++ri) {
        uint64_t RegisterToCheck = (static_cast<uint64_t>(Class) << 32) + ri;
          if (!DoesNodeSetInterfereWithRegister(Graph, Nodes, RegisterToCheck)) {
            RegAndClass = RegisterToCheck;
            break;
          }
      }

      // If we failed to find a virtual register then allocate more space for them
      if (RegAndClass == ~0ULL) {
        RegAndClass = (static_cast<uint64_t>(Class) << 32);
        RegAndClass |= AllocateMoreRegisters(Graph, Class);
      }

      TopRAPressure[Class] = std::max((uint32_t)RegAndClass, TopRAPressure[Class]);

      // Walk the partners and ensure they are all set to the same register now
      for (auto Partner : Nodes) {
        LogMan::Msg::D("Allocated reg 0x%lx for Tie value", RegAndClass);
        Partner->Head.RegAndClass = RegAndClass;
      }
    };

    // Very specifically the SSA count because we may have stuffed some additional data at the end of the register node graph
    uint32_t SSACount = IR->GetSSACount();

    // First thing, spin through an allocate any pinned registers
    for (uint32_t i = Graph->NodeCount; i > SSACount; --i) {
      uint32_t NodeIndex = i - 1;
      RegisterNode *CurrentNode = &Graph->Nodes[NodeIndex];

      if (CurrentNode->Head.PinnedReg) {
        FindPinnedRegForClass(CurrentNode, CurrentNode->Head.RegAndClass);
        continue;
      }

      LogMan::Msg::D("[Alloc] Node %d has class %d", NodeIndex, CurrentNode->Head.RegAndClass >> 32);
      // If this wasn't pinned then this is a temp
      FEXCore::IR::RegisterClassType RegClass = FEXCore::IR::RegisterClassType{uint32_t(CurrentNode->Head.RegAndClass >> 32)};
      FindRegForClass(RegClass, CurrentNode);
    }

    for (uint32_t i = SSACount; i > 0; --i) {
      uint32_t NodeIndex = i - 1;
      RegisterNode *CurrentNode = &Graph->Nodes[NodeIndex];

      auto CodeIter = IR->at(FEXCore::IR::OrderedNodeWrapper::WrapID(NodeIndex));
      auto WrapperOp = CodeIter();
      FEXCore::IR::OrderedNode *RealNode = WrapperOp->GetNode(ListBegin);
      FEXCore::IR::IROp_Header const *IROp = RealNode->Op(DataBegin);

      FEXCore::IR::Arch::OpConstraints Constraint = FEXCore::IR::Arch::GetOpConstraints(IROp->Op);

      FEXCore::IR::RegisterClassType RegClass = FEXCore::IR::RegisterClassType{uint32_t(CurrentNode->Head.RegAndClass >> 32)};

      if (CurrentNode->Head.TiePartner) {
        std::vector<RegisterNode *> Nodes;
        auto CurrentPartner = CurrentNode;
        while (CurrentPartner) {
          Nodes.emplace_back(CurrentPartner);
          CurrentPartner = CurrentPartner->Head.TiePartner;
        }

        FindRegForNodeSet(RegClass, Nodes);
      }
      else {
        if (Constraint.Flags & FEXCore::IR::Constraint_Needs_Temps) {
          LogMan::Msg::D("Found something that needs temps");
          if (Constraint.NumTempsGPR) {
            for (uint32_t temp = 0; temp < Constraint.NumTempsGPR; ++temp) {
              RegisterNode *RANode = &Graph->Nodes[CurrentNode->Head.TempsBase + temp];
              auto &LiveRange = LiveRanges.at(CurrentNode->Head.TempsBase + temp);

              LogMan::Msg::D("\tAllocated GPR: 0x%lx (Reg%d) for Temp %d", RANode->Head.RegAndClass, RANode->Head.RegAndClass, temp);
              LogMan::Msg::D("\tssa%d with temp range [%d, %d)", NodeIndex, LiveRange.Begin, LiveRange.End);
              LogMan::Msg::D("\tLiveRange Count: %d, Graph Count: %d", LiveRanges.size(), Graph->NodeCount);
            }
          }
          //if (Constraint.NumTempsFPR) {
          //  for (uint32_t temp = 0; temp < Constraint.NumTempsGPR; ++temp) {
          //    RegisterNode *TempNode = &Graph->Nodes[temp + Constraint.NumTempsGPR + 1];
          //    FindRegForClass(FEXCore::IR::FPRClass, TempNode);
          //  }
          //}
          //if (Constraint.NumTempsGPRPair) {
          //  for (uint32_t temp = 0; temp < Constraint.NumTempsGPR; ++temp) {
          //    FindRegForClass(FEXCore::IR::GPRPairClass, CurrentNode, temp + Constraint.NumTempsGPR + Constraint.NumTempsFPR + 1);
          //  }
          //}
        }

        if (CurrentNode->Head.PinnedReg) {
          FindPinnedRegForClass(CurrentNode, CurrentNode->Head.RegAndClass);
        }
        else if (CurrentNode->Head.RegAndClass != INVALID_REGCLASS &&
                 (uint32_t)CurrentNode->Head.RegAndClass == INVALID_REG) {
          // If this node wasn't already assigned then assign it now
          FindRegForClass(RegClass, CurrentNode);
        }
      }
    }
  }

  bool ConstraintRAPass::Run(FEXCore::IR::OpDispatchBuilder *Disp) {
    bool Changed = false;

    // We need to rerun compaction every step
    Changed |= LocalCompaction->Run(Disp);

    auto IR = Disp->ViewIR();
    uint32_t SSACount = IR.GetSSACount();

    TopRAPressure.assign(TopRAPressure.size(), 0);
    ResetRegisterGraph(Graph, SSACount);
    FindNodeClasses(Graph, &IR);
    CalculateLiveRange(Graph, &IR, &LiveRanges);
    CalculateNodeInterference(Graph, &IR, &LiveRanges);

    AllocateVirtualRegisters(&IR);

    HadFullRA = true;
    Disp->ShouldDump = true;

    for (size_t i = 0; i < PhysicalRegisterCount.size(); ++i) {
      // Virtual registers fit completely within physical registers
      // Remap virtual 1:1 to physical
      HadFullRA &= TopRAPressure[i] < PhysicalRegisterCount[i];
    }

    if (!HadFullRA) {
      LogMan::Msg::D("Need to spill registers\n");
      Disp->CTX->ShouldStop = true;
    }

    return Changed;
  }

}

namespace FEXCore::IR {
  FEXCore::IR::RegisterAllocationPass* CreateNewRAPass() {
    return new ConstraintRAPass{};
  }
}
