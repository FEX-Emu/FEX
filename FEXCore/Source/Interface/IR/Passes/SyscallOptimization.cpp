/*
$info$
tags: ir|opts
desc: Removes unused arguments if known syscall number
$end_info$
*/

#include "Interface/IR/PassManager.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IREmitter.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/Profiler.h>

#include <memory>
#include <stdint.h>

namespace FEXCore::IR {

class SyscallOptimization final : public FEXCore::IR::Pass {
public:
  bool Run(IREmitter *IREmit) override;
};

bool SyscallOptimization::Run(IREmitter *IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::SyscallOpt");

  bool Changed = false;
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
            auto InlineSyscall = IREmit->_InlineSyscall(
              CurrentIR.GetNode(IROp->Args[1]),
              CurrentIR.GetNode(IROp->Args[2]),
              CurrentIR.GetNode(IROp->Args[3]),
              CurrentIR.GetNode(IROp->Args[4]),
              CurrentIR.GetNode(IROp->Args[5]),
              CurrentIR.GetNode(IROp->Args[6]),
              SyscallDef.HostSyscallNumber,
              Op->Flags);

            // Replace all syscall uses with this inline one
            IREmit->ReplaceAllUsesWith(CodeNode, InlineSyscall);

            // We must remove here since DCE can't remove a IROp with sideeffects
            IREmit->Remove(CodeNode);
          }
#endif
        }

        Changed = true;
      }
    }
  }

  return Changed;
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateSyscallOptimization() {
  return fextl::make_unique<SyscallOptimization>();
}

}
