#pragma once
#include "Interface/IR/PassManager.h"
#include <FEXCore/IR/RegisterAllocationData.h>
#include <vector>

namespace FEXCore::IR {
class IRListView;

class RegisterAllocationPass : public FEXCore::IR::Pass {
  public:
    bool HasFullRA() const { return HadFullRA; }

    virtual void AllocateRegisterSet(uint32_t RegisterCount, uint32_t ClassCount) = 0;
    virtual void AddRegisters(FEXCore::IR::RegisterClassType Class, uint32_t RegisterCount) = 0;

    /**
     * @brief Adds a conflict between the two registers (and their respective classes) so if one is allocated then the other can not be allocated in
     * the same live range.
     *
     * Conflict is added both directions, so only necessary to add a conflict one way
     *
     * ex:
     * AddRegisters(GPRClass, 2);  -> {x0, x1} added to register class GPR
     * AddRegisters(PairClass, 1); -> {{x0, x1}} pair added to class GPR
     * AddRegisterConflict(GPRClass, 0, PairClass, 0); -> Make sure the pair interferes with x0
     * AddRegisterConflict(GPRClass, 1, PairClass, 1); -> Make sure the pair interferes with x1
     */
    virtual void AddRegisterConflict(FEXCore::IR::RegisterClassType ClassConflict, uint32_t RegConflict, FEXCore::IR::RegisterClassType Class, uint32_t Reg) = 0;

    /**
     * @name Inference graph handling
     * @{ */

    /**
     * @brief Returns the register and class map array
     */
    virtual RegisterAllocationData *GetAllocationData() = 0;

    /**
     * @brief Returns and transfers ownership of the register and class map array
     */
    virtual std::unique_ptr<RegisterAllocationData, RegisterAllocationDataDeleter> PullAllocationData() = 0;
    /**  @} */

  protected:
    bool HasSpills {};
    uint32_t SpillSlotCount {};
    bool HadFullRA {};
};

}
