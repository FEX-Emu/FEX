// SPDX-License-Identifier: MIT
#pragma once
#include "IR.h"
#include <FEXCore/Core/Context.h>
#include <FEXCore/Utils/Allocator.h>
#include <cstring>

namespace FEXCore::IR {

union PhysicalRegister {
  uint8_t Raw;
  struct {
    // 32 maximum physical registers
    uint8_t Reg : 5;
    // 8 Maximum classes
    uint8_t Class : 3;
  };

  bool operator==(const PhysicalRegister& Other) const {
    return Raw == Other.Raw;
  }

  PhysicalRegister(RegisterClassType Class, uint8_t Reg)
    : Reg(Reg)
    , Class(Class.Val) {}

  PhysicalRegister(OrderedNodeWrapper Arg)
    : Raw(Arg.GetImmediate()) {}

  PhysicalRegister(Ref Node)
    : Raw(Node->Reg) {}

  static const PhysicalRegister Invalid() {
    return PhysicalRegister(InvalidClass, InvalidReg);
  }

  bool IsInvalid() const {
    return *this == Invalid();
  }
};

static_assert(sizeof(PhysicalRegister) == 1);

struct RegisterAllocationDataDeleter;

// This class is serialized, can't have any holes in the structure
// otherwise ASAN complains about reading uninitialized memory
class FEX_PACKED RegisterAllocationData {
public:
  uint32_t SpillSlotCount {};
  uint32_t MapCount {};
  PhysicalRegister Map[0];

  PhysicalRegister GetNodeRegister(NodeID Node) const {
    return Map[Node.Value];
  }
  uint32_t SpillSlots() const {
    return SpillSlotCount;
  }

  static size_t Size(uint32_t NodeCount) {
    return sizeof(RegisterAllocationData) + NodeCount * sizeof(Map[0]);
  }

  using UniquePtr = std::unique_ptr<FEXCore::IR::RegisterAllocationData, RegisterAllocationDataDeleter>;

  static UniquePtr Create(uint32_t NodeCount);

  UniquePtr CreateCopy() const;

  void Serialize(FEXCore::Context::AOTIRWriter& stream) const {
    stream.Write((const char*)&SpillSlotCount, sizeof(SpillSlotCount));
    stream.Write((const char*)&MapCount, sizeof(MapCount));
    // RAData (inline)
    stream.Write((const char*)&Map[0], sizeof(Map[0]) * MapCount);
  }
};

struct RegisterAllocationDataDeleter {
  void operator()(RegisterAllocationData* r) const {
    FEXCore::Allocator::free(r);
  }
};

inline auto RegisterAllocationData::Create(uint32_t NodeCount) -> UniquePtr {
  auto Ret = (RegisterAllocationData*)FEXCore::Allocator::malloc(Size(NodeCount));
  memset(&Ret->Map[0], PhysicalRegister::Invalid().Raw, NodeCount);
  Ret->SpillSlotCount = 0;
  Ret->MapCount = NodeCount;
  return UniquePtr {Ret};
}

inline auto RegisterAllocationData::CreateCopy() const -> UniquePtr {
  auto copy = (RegisterAllocationData*)FEXCore::Allocator::malloc(Size(MapCount));
  memcpy((void*)&copy->Map[0], (void*)&Map[0], MapCount * sizeof(Map[0]));
  copy->SpillSlotCount = SpillSlotCount;
  copy->MapCount = MapCount;
  return UniquePtr {copy};
}

} // namespace FEXCore::IR
