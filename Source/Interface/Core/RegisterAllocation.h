#pragma once
#include <stdint.h>
#include <vector>

namespace FEXCore::RA {
struct RegisterSet;
struct RegisterGraph;
using CrappyBitset = std::vector<bool>;

RegisterSet *AllocateRegisterSet(uint32_t RegisterCount, uint32_t ClassCount);
void FreeRegisterSet(RegisterSet *Set);
void AddRegisters(RegisterSet *Set, uint32_t Class, uint32_t RegistersBase, uint32_t RegisterCount);

/**
 * @name Inference graph handling
 * @{ */

RegisterGraph *AllocateRegisterGraph(RegisterSet *Set, uint32_t NodeCount);
void FreeRegisterGraph(RegisterGraph *Graph);
void ResetRegisterGraph(RegisterGraph *Graph, uint32_t NodeCount);
void SetNodeClass(RegisterGraph *Graph, uint32_t Node, uint32_t Class);
void AddNodeInterference(RegisterGraph *Graph, uint32_t Node1, uint32_t Node2);
uint32_t GetNodeRegister(RegisterGraph *Graph, uint32_t Node);

bool AllocateRegisters(RegisterGraph *Graph);

/**  @} */

}

