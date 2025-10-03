// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
$end_info$
*/

#pragma once
#include "Interface/IR/PassManager.h"

#include <memory>
#include <stdint.h>

namespace FEXCore::IR {
enum class RegClass : uint32_t;

class RegisterAllocationPass : public FEXCore::IR::Pass {
public:
  virtual void AddRegisters(RegClass Class, uint32_t RegisterCount) = 0;

  // Number of GPRs usable for pairs at start of GPR set. Must be even.
  uint32_t PairRegs;
};

} // namespace FEXCore::IR
