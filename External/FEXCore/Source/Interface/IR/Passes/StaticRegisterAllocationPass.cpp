#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

// Higher values might result in more stores getting eliminated but will make the optimization take more time

namespace FEXCore::IR {

class StaticRegisterAllocationPass final : public FEXCore::IR::Pass {
public:
  bool Run(IREmitter *IREmit) override;
};

bool IsStaticAllocGpr(uint32_t Offset, RegisterClassType Class) {
  bool rv = false;
  auto begin = offsetof(FEXCore::Core::ThreadState, State.gregs[0]);
  auto end = offsetof(FEXCore::Core::ThreadState, State.gregs[17]);

  if (Offset >= begin && Offset < end) {
    auto reg = (Offset - begin) / 8;
    assert(Class == IR::GPRClass);

    rv = reg < 16; // 0..15 -> 16 in total
  }

  return rv;
}

bool IsStaticAllocFpr(uint32_t Offset, RegisterClassType Class, bool AllowGpr) {
  bool rv = false;
  auto begin = offsetof(FEXCore::Core::ThreadState, State.xmm[0][0]);
  auto end = offsetof(FEXCore::Core::ThreadState, State.xmm[17][0]);

  if (Offset >= begin && Offset < end) {
    auto reg = (Offset - begin)/16;
    assert(Class == IR::FPRClass || (AllowGpr && Class == IR::GPRClass));

    rv = reg < 16; // 0..15 -> 16 in total
  }

  return rv;
}
/**
 * @brief This is a temporary pass to detect simple multiblock dead GPR stores
 *
 * First pass computes which GPRs are read and written per block
 *
 * Second pass computes which GPRs are stored, but overwritten by the next block(s).
 * It also propagates this information a few times to catch dead GPRs across multiple blocks.
 *
 * Third pass removes the dead stores.
 *
 */
bool StaticRegisterAllocationPass::Run(IREmitter *IREmit) {
  auto CurrentIR = IREmit->ViewIR();

  if (CurrentIR.GetHeader()->ShouldInterpret)
    return false;

  for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
        IREmit->SetWriteCursor(CodeNode);
        
        if (IROp->Op == OP_LOADCONTEXT) {
            auto Op = IROp->CW<IR::IROp_LoadContext>();

            if (IsStaticAllocGpr(Op->Offset, Op->Class) || IsStaticAllocFpr(Op->Offset, Op->Class, true)) {
              auto GeneralClass = Op->Class;
              if (IsStaticAllocFpr(Op->Offset, GeneralClass, true) && GeneralClass == GPRClass) {
                GeneralClass = FPRClass;
              }
              auto StaticClass = GeneralClass == GPRClass ? GPRFixedClass : FPRFixedClass;
              OrderedNode *sraReg = IREmit->_LoadRegister(false, Op->Offset, GeneralClass, StaticClass, Op->Header.Size);
              if (GeneralClass != Op->Class) {
                sraReg = IREmit->_VExtractToGPR(Op->Header.Size, Op->Header.Size, sraReg, 0);
              }
              IREmit->ReplaceAllUsesWith(CodeNode, sraReg);
            }
        } if (IROp->Op == OP_STORECONTEXT) {
            auto Op = IROp->CW<IR::IROp_StoreContext>();

            if (IsStaticAllocGpr(Op->Offset, Op->Class) || IsStaticAllocFpr(Op->Offset, Op->Class, true)) {
              auto val = IREmit->UnwrapNode(Op->Value);

              auto GeneralClass = Op->Class;
              if (IsStaticAllocFpr(Op->Offset, GeneralClass, true) && GeneralClass == GPRClass) {
                val = IREmit->_VCastFromGPR(Op->Header.Size, Op->Header.Size, val);
                GeneralClass = FPRClass;
              }

              auto StaticClass = GeneralClass == GPRClass ? GPRFixedClass : FPRFixedClass;
              OrderedNode *sraReg = IREmit->_StoreRegister(val, false, Op->Offset, GeneralClass, StaticClass, Op->Header.Size);

              IREmit->Remove(CodeNode);
            }
        }
    }
  }

  return true;
}

FEXCore::IR::Pass* CreateStaticRegisterAllocationPass() {
  return new StaticRegisterAllocationPass{};
}

}
