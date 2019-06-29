#include "Common/BitSet.h"
#include "Interface/Core/RegisterAllocation.h"

#include <LogManager.h>

#include <vector>

constexpr uint32_t INVALID_REG = ~0U;
constexpr uint32_t INVALID_CLASS = ~0U;


namespace FEXCore::RA {

  struct Register {
  };

  struct RegisterClass {
    uint32_t RegisterBase;
    uint32_t NumberOfRegisters{0};
    BitSet<uint32_t> Registers;
  };

  struct RegisterNode {
    uint32_t RegisterClass;
    uint32_t Register;
    uint32_t InterferenceCount;
    uint32_t InterferenceListSize;
    uint32_t *InterferenceList;
    BitSet<uint32_t> Interference;
  };

  static_assert(std::is_pod<RegisterNode>::value, "We want this to be POD");

  struct RegisterSet {
    Register *Registers;
    RegisterClass *RegisterClasses;
    uint32_t RegisterCount;
    uint32_t ClassCount;
  };

  struct SpillStackUnit {
    uint32_t Node;
    uint32_t Class;
  };

  struct RegisterGraph {
    RegisterSet *Set;
    RegisterNode *Nodes;
    uint32_t NodeCount;
    uint32_t MaxNodeCount;
    std::vector<SpillStackUnit> SpillStack;
  };

  RegisterSet *AllocateRegisterSet(uint32_t RegisterCount, uint32_t ClassCount) {
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

  void FreeRegisterSet(RegisterSet *Set) {
    for (uint32_t i = 0; i < Set->ClassCount; ++i) {
      Set->RegisterClasses[i].Registers.Free();
    }
    free(Set->RegisterClasses);
    free(Set->Registers);
    delete Set;
  }

  void AddRegisters(RegisterSet *Set, uint32_t Class, uint32_t RegistersBase, uint32_t RegisterCount) {
    for (uint32_t i = 0; i < RegisterCount; ++i) {
      Set->RegisterClasses[Class].Registers.Set(RegistersBase + i);
    }
    Set->RegisterClasses[Class].RegisterBase = RegistersBase;
    Set->RegisterClasses[Class].NumberOfRegisters += RegisterCount;
  }

  RegisterGraph *AllocateRegisterGraph(RegisterSet *Set, uint32_t NodeCount) {
    RegisterGraph *Graph = new RegisterGraph;
    Graph->Set = Set;
    Graph->NodeCount = NodeCount;
    Graph->MaxNodeCount = NodeCount;
    Graph->Nodes = static_cast<RegisterNode*>(calloc(NodeCount, sizeof(RegisterNode)));

    // Initialize nodes
    for (uint32_t i = 0; i < NodeCount; ++i) {
      Graph->Nodes[i].Register = INVALID_REG;
      Graph->Nodes[i].RegisterClass = INVALID_CLASS;
      Graph->Nodes[i].InterferenceListSize = 32;
      Graph->Nodes[i].InterferenceList = reinterpret_cast<uint32_t*>(calloc(Graph->Nodes[i].InterferenceListSize, sizeof(uint32_t)));
      Graph->Nodes[i].InterferenceCount = 0;
      Graph->Nodes[i].Interference.Allocate(NodeCount);
      Graph->Nodes[i].Interference.Clear(NodeCount);
    }

    return Graph;
  }

  void ResetRegisterGraph(RegisterGraph *Graph, uint32_t NodeCount) {
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
        Graph->Nodes[i].InterferenceListSize = 32;
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

  void FreeRegisterGraph(RegisterGraph *Graph) {
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

  void SetNodeClass(RegisterGraph *Graph, uint32_t Node, uint32_t Class) {
    Graph->Nodes[Node].RegisterClass = Class;
  }

  void AddNodeInterference(RegisterGraph *Graph, uint32_t Node1, uint32_t Node2) {
    auto AddInterference = [&Graph](uint32_t Node1, uint32_t Node2) {
      RegisterNode *Node = &Graph->Nodes[Node1];
      Node->Interference.Set(Node2);
      if (Node->InterferenceListSize <= Node->InterferenceCount) {
        Node->InterferenceListSize *= 2;
        Node->InterferenceList = reinterpret_cast<uint32_t*>(realloc(Node->InterferenceList, Node->InterferenceListSize * sizeof(uint32_t)));
      }
      Node->InterferenceList[Node->InterferenceCount] = Node2;
      ++Node->InterferenceCount;
    };

    AddInterference(Node1, Node2);
    AddInterference(Node2, Node1);
  }

  uint32_t GetNodeRegister(RegisterGraph *Graph, uint32_t Node) {
    return Graph->Nodes[Node].Register;
  }

  static bool HasInterference(RegisterGraph *Graph, RegisterNode *Node, uint32_t Register) {
    for (uint32_t i = 0; i < Node->InterferenceCount; ++i) {
      RegisterNode *IntNode = &Graph->Nodes[Node->InterferenceList[i]];
      if (IntNode->Register == Register) {
        return true;
      }
    }

    return false;
  }

  bool AllocateRegisters(RegisterGraph *Graph) {
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
        Graph->SpillStack.emplace_back(SpillStackUnit{i, CurrentNode->RegisterClass});
      }
      else {
        CurrentNode->Register = RAClass->RegisterBase + Reg;
      }
    }

    if (!Graph->SpillStack.empty()) {
      printf("Couldn't allocate %ld registers\n", Graph->SpillStack.size());
      return false;
    }
    return true;
  }

}

