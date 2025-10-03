// SPDX-License-Identifier: MIT
#pragma once

#include "IR.h"
#include <cstdint>

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

  PhysicalRegister(RegClass Class, uint8_t Reg)
    : Reg(Reg)
    , Class(uint8_t(Class)) {}

  PhysicalRegister(OrderedNodeWrapper Arg)
    : Raw(Arg.GetImmediate()) {}

  PhysicalRegister(Ref Node)
    : Raw(Node->Reg) {}

  RegClass AsRegClass() const {
    return RegClass {Class};
  }

  static const PhysicalRegister Invalid() {
    return PhysicalRegister(RegClass::Invalid, 0);
  }

  bool IsInvalid() const {
    static_assert(uint8_t(RegClass::Invalid) == 0);
    return Raw == 0;
  }
};

static_assert(sizeof(PhysicalRegister) == 1);

} // namespace FEXCore::IR
