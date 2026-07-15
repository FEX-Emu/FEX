// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
$end_info$
*/

#pragma once

#include "Interface/IR/PassManager.h"

#include <cstdint>
#include <memory>

namespace FEXCore::IR {
enum class RegClass : uint32_t;

class RegisterAllocationPass : public FEXCore::IR::Pass {
public:
  virtual void AddRegisters(RegClass Class, uint32_t RegisterCount) = 0;

  void SetNumPairRegs(uint32_t NumRegs);

protected:
  // Number of GPRs usable for pairs at start of GPR set. Must be even.
  uint32_t PairRegs {};
};

} // namespace FEXCore::IR
