/*
$info$
tags: ir|opts
desc: Replaces Load/StoreContext with Load/StoreReg for SRA regs
$end_info$
*/

#include "Interface/IR/PassManager.h"
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IREmitter.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>

#include <memory>
#include <stddef.h>
#include <stdint.h>

namespace FEXCore::IR {

class StaticRegisterAllocationPass final : public FEXCore::IR::Pass {
public:
  explicit StaticRegisterAllocationPass(bool SupportsAVX_) : SupportsAVX{SupportsAVX_} {}

  bool Run(IREmitter *IREmit) override;

private:
  bool SupportsAVX;

  bool IsStaticAllocGpr(uint32_t Offset, RegisterClassType Class) const {
    const auto begin = offsetof(Core::CPUState, gregs[0]);
    const auto end = offsetof(Core::CPUState, gregs[16]);

    if (Offset >= begin && Offset < end) {
      const auto reg = (Offset - begin) / Core::CPUState::GPR_REG_SIZE;
      LOGMAN_THROW_AA_FMT(Class.Val == IR::GPRClass.Val, "unexpected Class {}", Class);

      // 0..15 -> 16 in total
      return reg < Core::CPUState::NUM_GPRS;
    }

    return false;
  }

  bool IsStaticAllocFpr(uint32_t Offset, RegisterClassType Class, bool AllowGpr) const {
    const auto [begin, end] = [this]() -> std::pair<ptrdiff_t, ptrdiff_t> {
      if (SupportsAVX) {
        return {
          offsetof(Core::CPUState, xmm.avx.data[0][0]),
          offsetof(Core::CPUState, xmm.avx.data[16][0]),
        };
      } else {
        return {
          offsetof(Core::CPUState, xmm.sse.data[0][0]),
          offsetof(Core::CPUState, xmm.sse.data[16][0]),
        };
      }
    }();

    if (Offset >= begin && Offset < end) {
      const auto size = SupportsAVX ? Core::CPUState::XMM_AVX_REG_SIZE
                                    : Core::CPUState::XMM_SSE_REG_SIZE;
      const auto reg = (Offset - begin) / size;
      LOGMAN_THROW_AA_FMT(Class.Val == IR::FPRClass.Val || (AllowGpr && Class.Val == IR::GPRClass.Val), "unexpected Class {}, AllowGpr {}", Class, AllowGpr);

      // 0..15 -> 16 in total
      return reg < Core::CPUState::NUM_XMMS;
    }

    return false;
  }
};

/**
 * @brief This pass replaces Load/Store Context with Load/Store Register for Statically Mapped registers. It also does some validation.
 *
 */
bool StaticRegisterAllocationPass::Run(IREmitter *IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::SRA");

  auto CurrentIR = IREmit->ViewIR();

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
              IREmit->_StoreRegister(val, false, Op->Offset, GeneralClass, StaticClass, Op->Header.Size);

              IREmit->Remove(CodeNode);
            }
        }
    }
  }

  return true;
}

std::unique_ptr<FEXCore::IR::Pass> CreateStaticRegisterAllocationPass(bool SupportsAVX) {
  return std::make_unique<StaticRegisterAllocationPass>(SupportsAVX);
}

}
