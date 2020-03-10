#include "Interface/IR/PassManager.h"
#include <vector>

namespace FEXCore::IR {
template<bool>
class IRListView;

class RegisterAllocationPass : public FEXCore::IR::Pass {
  public:
    static constexpr uint32_t GPRClass = 0;
    static constexpr uint32_t FPRClass = 1;
    static constexpr uint32_t FLAGSClass = 2;

    bool HasFullRA() const { return HadFullRA; }
    uint32_t SpillSlots() const { return SpillSlotCount; }

    virtual void AllocateRegisterSet(uint32_t RegisterCount, uint32_t ClassCount) = 0;
    virtual void AddRegisters(uint32_t Class, uint32_t RegisterCount) = 0;

    /**
     * @name Inference graph handling
     * @{ */

    /**
     * @brief Returns the register and class encoded together
     * Top 32bits is the class, lower 32bits is the register
     */
    virtual uint64_t GetNodeRegister(uint32_t Node) = 0;
    /**  @} */

  protected:
    bool HasSpills {};
    uint32_t SpillSlotCount {};
    bool HadFullRA {};
};

}
