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

} // namespace FEXCore::IR
