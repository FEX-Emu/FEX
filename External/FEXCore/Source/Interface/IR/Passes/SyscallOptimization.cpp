#include "Interface/Context/Context.h"
#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include "LogManager.h"

namespace FEXCore::IR {

class SyscallOptimization final : public FEXCore::IR::Pass {
public:
  bool Run(IREmitter *IREmit) override;
};

bool SyscallOptimization::Run(IREmitter *IREmit) {
  bool Changed = false;
  auto CurrentIR = IREmit->ViewIR();


  for (auto [CodeNode, IROp] : CurrentIR.GetAllCode()) {

    if (IROp->Op == FEXCore::IR::OP_SYSCALL) {
      // Is the first argument a constant?
      uint64_t Constant;
      if (IREmit->IsValueConstant(IROp->Args[0], &Constant)) {
        auto SyscallDef = Manager->SyscallHandler->GetDefinition(Constant);
        // XXX: Once we have the ability to do real function calls then we can call directly in to the syscall handler
        if (SyscallDef->NumArgs < FEXCore::HLE::SyscallArguments::MAX_ARGS) {
          // If the number of args are less than what the IR op supports then we can remove arg usage
          // We need +1 since we are still passing in syscall number here
          for (uint8_t Arg = (SyscallDef->NumArgs + 1); Arg < FEXCore::HLE::SyscallArguments::MAX_ARGS; ++Arg) {
            IREmit->ReplaceNodeArgument(CodeNode, Arg, IREmit->Invalid());
          }
          Changed = true;
        }
      }
    }
  }


  return Changed;
}

FEXCore::IR::Pass* CreateSyscallOptimization() {
  return new SyscallOptimization{};
}

}
