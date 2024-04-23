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
  virtual void AllocateRegisterSet(uint32_t ClassCount) = 0;
  virtual void AddRegisters(FEXCore::IR::RegisterClassType Class, uint32_t RegisterCount) = 0;

  /**
   * @name Inference graph handling
   * @{ */

  /**
   * @brief Returns the register and class map array
   */
  virtual RegisterAllocationData* GetAllocationData() = 0;

  /**
   * @brief Returns and transfers ownership of the register and class map array
   */
  virtual std::unique_ptr<RegisterAllocationData, RegisterAllocationDataDeleter> PullAllocationData() = 0;
  /**  @} */
};

} // namespace FEXCore::IR
