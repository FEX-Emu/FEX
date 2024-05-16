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
class RegisterAllocationData;
struct RegisterAllocationDataDeleter;
struct RegisterClassType;

class RegisterAllocationPass : public FEXCore::IR::Pass {
public:
  virtual void AddRegisters(FEXCore::IR::RegisterClassType Class, uint32_t RegisterCount) = 0;

  // Number of GPRs usable for pairs at start of GPR set. Must be even.
  uint32_t PairRegs;

  /**
   * @brief Returns the register and class map array
   */
  virtual RegisterAllocationData* GetAllocationData() = 0;

  /**
   * @brief Returns and transfers ownership of the register and class map array
   */
  virtual std::unique_ptr<RegisterAllocationData, RegisterAllocationDataDeleter> PullAllocationData() = 0;
};

} // namespace FEXCore::IR
