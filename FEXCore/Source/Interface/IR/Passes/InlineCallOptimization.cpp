// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
desc: Removes unused arguments if known syscall number
$end_info$
*/

#include "Interface/Core/CPUID.h"
#include "Interface/IR/IREmitter.h"
#include "Interface/IR/PassManager.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/Profiler.h>

#include <memory>
#include <stdint.h>

namespace FEXCore::IR {

class InlineCallOptimization final : public FEXCore::IR::Pass {
public:
  InlineCallOptimization(const FEXCore::CPUIDEmu* CPUID)
    : CPUID {CPUID} {}
  void Run(IREmitter* IREmit) override;
private:
  const FEXCore::CPUIDEmu* CPUID;
};

void InlineCallOptimization::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::SyscallOpt");

  auto CurrentIR = IREmit->ViewIR();

  for (auto [CodeNode, IROp] : CurrentIR.GetAllCode()) {

    if (IROp->Op == FEXCore::IR::OP_SYSCALL) {
      auto Op = IROp->CW<IR::IROp_Syscall>();

      // Is the first argument a constant?
      uint64_t Constant;
      if (IREmit->IsValueConstant(Op->SyscallID, &Constant)) {
        auto SyscallDef = Manager->SyscallHandler->GetSyscallABI(Constant);
        auto SyscallFlags = Manager->SyscallHandler->GetSyscallFlags(Constant);

        // Update the syscall flags
        Op->Flags = SyscallFlags;

        // XXX: Once we have the ability to do real function calls then we can call directly in to the syscall handler
        if (SyscallDef.NumArgs < FEXCore::HLE::SyscallArguments::MAX_ARGS) {
          // If the number of args are less than what the IR op supports then we can remove arg usage
          // We need +1 since we are still passing in syscall number here
          for (uint8_t Arg = (SyscallDef.NumArgs + 1); Arg < FEXCore::HLE::SyscallArguments::MAX_ARGS; ++Arg) {
            IREmit->ReplaceNodeArgument(CodeNode, Arg, IREmit->Invalid());
          }
#ifdef _M_ARM_64
          // Replace syscall with inline passthrough syscall if we can
          if (SyscallDef.HostSyscallNumber != -1) {
            IREmit->SetWriteCursor(CodeNode);
            // Skip Args[0] since that is the syscallid
            auto InlineSyscall =
              IREmit->_InlineSyscall(CurrentIR.GetNode(IROp->Args[1]), CurrentIR.GetNode(IROp->Args[2]), CurrentIR.GetNode(IROp->Args[3]),
                                     CurrentIR.GetNode(IROp->Args[4]), CurrentIR.GetNode(IROp->Args[5]), CurrentIR.GetNode(IROp->Args[6]),
                                     SyscallDef.HostSyscallNumber, Op->Flags);

            // Replace all syscall uses with this inline one
            IREmit->ReplaceAllUsesWith(CodeNode, InlineSyscall);

            // We must remove here since DCE can't remove a IROp with sideeffects
            IREmit->Remove(CodeNode);
          }
#endif
        }
      }
    } else if (IROp->Op == FEXCore::IR::OP_CPUID) {
      auto Op = IROp->CW<IR::IROp_CPUID>();

      uint64_t ConstantFunction {}, ConstantLeaf {};
      bool IsConstantFunction = IREmit->IsValueConstant(Op->Function, &ConstantFunction);
      bool IsConstantLeaf = IREmit->IsValueConstant(Op->Leaf, &ConstantLeaf);
      // If the CPUID function is constant then we can try and optimize.
      if (IsConstantFunction) { // && ConstantFunction != 1) {
        // Check if it supports constant data reporting for this function.
        const auto SupportsConstant = CPUID->DoesFunctionReportConstantData(ConstantFunction);
        if (SupportsConstant.SupportsConstantFunction == CPUIDEmu::SupportsConstant::CONSTANT) {
          // If the CPUID needs a constant leaf to be optimized then this can't work if we didn't const-prop the leaf register.
          if (!(SupportsConstant.NeedsLeaf == CPUIDEmu::NeedsLeafConstant::NEEDSLEAFCONSTANT && !IsConstantLeaf)) {
            // Calculate the constant data and replace all uses.
            // DCE will remove the CPUID IR operation.
            const auto ConstantCPUIDResult = CPUID->RunFunction(ConstantFunction, ConstantLeaf);
            uint64_t ResultsLower = (static_cast<uint64_t>(ConstantCPUIDResult.ebx) << 32) | ConstantCPUIDResult.eax;
            uint64_t ResultsUpper = (static_cast<uint64_t>(ConstantCPUIDResult.edx) << 32) | ConstantCPUIDResult.ecx;
            IREmit->SetWriteCursor(CodeNode);
            auto ElementPair = IREmit->_CreateElementPair(IR::OpSize::i128Bit, IREmit->_Constant(ResultsLower), IREmit->_Constant(ResultsUpper));
            // Replace all CPUID uses with this inline one
            IREmit->ReplaceAllUsesWith(CodeNode, ElementPair);
          }
        }
      }
    }

    else if (IROp->Op == FEXCore::IR::OP_XGETBV) {
      auto Op = IROp->CW<IR::IROp_XGetBV>();

      uint64_t ConstantFunction {};
      if (IREmit->IsValueConstant(Op->Function, &ConstantFunction) && CPUID->DoesXCRFunctionReportConstantData(ConstantFunction)) {
        const auto ConstantXCRResult = CPUID->RunXCRFunction(ConstantFunction);
        IREmit->SetWriteCursor(CodeNode);
        auto ElementPair =
          IREmit->_CreateElementPair(IR::OpSize::i64Bit, IREmit->_Constant(ConstantXCRResult.eax), IREmit->_Constant(ConstantXCRResult.edx));
        // Replace all xgetbv uses with this inline one
        IREmit->ReplaceAllUsesWith(CodeNode, ElementPair);
      }
    }
  }
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateInlineCallOptimization(const FEXCore::CPUIDEmu* CPUID) {
  return fextl::make_unique<InlineCallOptimization>(CPUID);
}

} // namespace FEXCore::IR
