#pragma once
#include "IR.h"

namespace FEXCore::IR {

union PhysicalRegister {
  uint8_t Raw;
  struct {
    uint8_t Reg: 5;
    uint8_t Class: 3;
  };

  bool operator==(const PhysicalRegister &Other) const {
    return Raw == Other.Raw;
  }

  PhysicalRegister(RegisterClassType Class, uint8_t Reg) : Reg(Reg), Class(Class.Val) { }

  static const PhysicalRegister Invalid() {
    return PhysicalRegister(InvalidClass, InvalidReg);
  }

  bool IsInvalid() {
    return *this == Invalid();
  }
};

static_assert(sizeof(PhysicalRegister) == 1);

class RegisterAllocationData;
struct RegisterAllocationDataDeleter {
  void operator()(RegisterAllocationData* r) {
    free(r);
  }
};

class RegisterAllocationData {
  public:
    uint32_t SpillSlotCount {};
    uint32_t MapCount {};
    PhysicalRegister Map[0];

    PhysicalRegister GetNodeRegister(uint32_t Node) const {
      return Map[Node];
    }
    uint32_t SpillSlots() const { return SpillSlotCount; }

    static size_t Size(uint32_t NodeCount) {
      return sizeof(RegisterAllocationData) + NodeCount * sizeof(Map[0]);
    }
};

} 
