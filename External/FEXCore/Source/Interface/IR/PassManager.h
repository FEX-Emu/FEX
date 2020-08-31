#pragma once

#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/IREmitter.h>

#include <functional>
#include <memory>
#include <vector>

namespace FEXCore {
class SyscallHandler;
}

namespace FEXCore::IR {
class OpDispatchBuilder;
class SyscallOptimization;

using ShouldExitHandler = std::function<void(void)>;

class Pass {
public:
  virtual ~Pass() = default;
  virtual bool Run(IREmitter *IREmit) = 0;

  void RegisterPassManager(PassManager *_Manager) {
    Manager = _Manager;
  }

protected:
  PassManager *Manager;
};

class PassManager final {
  friend class SyscallOptimization;
public:
  void AddDefaultPasses();
  void AddDefaultValidationPasses();
  void InsertPass(Pass *Pass) {
    Pass->RegisterPassManager(this);
    Passes.emplace_back(Pass);
  }
  void InsertRAPass(Pass *Pass) {
    RAPass = Pass;
    InsertPass(Pass);
  }

  bool Run(IREmitter *IREmit);

  void RegisterExitHandler(ShouldExitHandler Handler) {
    ExitHandler = Handler;
  }

  bool HasRAPass() const {
    return RAPass != nullptr;
  }

  IR::RegisterAllocationPass *GetRAPass() {
    return reinterpret_cast<IR::RegisterAllocationPass*>(RAPass);
  }

  void RegisterSyscallHandler(SyscallHandler *Handler) {
    SyscallHandler = Handler;
  }

protected:
  ShouldExitHandler ExitHandler;
  SyscallHandler *SyscallHandler;

private:
  Pass *RAPass{};

  std::vector<std::unique_ptr<Pass>> Passes;

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  std::vector<std::unique_ptr<Pass>> ValidationPasses;
  void InsertValidationPass(Pass *Pass) {
    Pass->RegisterPassManager(this);
    ValidationPasses.emplace_back(Pass);
  }
#endif
};
}

