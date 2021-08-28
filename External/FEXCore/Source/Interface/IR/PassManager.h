/*
$info$
tags: ir|opts
$end_info$
*/

#pragma once

#include <FEXCore/Config/Config.h>

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace FEXCore::HLE {
class SyscallHandler;
}

namespace FEXCore::IR {
class PassManager;
class IREmitter;
class RegisterAllocationPass;

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
  void AddDefaultPasses(bool InlineConstants, bool StaticRegisterAllocation);
  void AddDefaultValidationPasses();
  Pass* InsertPass(std::unique_ptr<Pass> Pass) {
    Pass->RegisterPassManager(this);
    return Passes.emplace_back(std::move(Pass)).get();
  }

  void InsertRegisterAllocationPass(bool OptimizeSRA);

  bool Run(IREmitter *IREmit);

  void RegisterExitHandler(ShouldExitHandler Handler) {
    ExitHandler = std::move(Handler);
  }

  bool HasRAPass() const {
    return RAPass != nullptr;
  }

  IR::RegisterAllocationPass *GetRAPass() {
    return reinterpret_cast<IR::RegisterAllocationPass*>(RAPass);
  }

  void RegisterSyscallHandler(FEXCore::HLE::SyscallHandler *Handler) {
    SyscallHandler = Handler;
  }

protected:
  ShouldExitHandler ExitHandler;
  FEXCore::HLE::SyscallHandler *SyscallHandler;

private:
  Pass *RAPass{};
  Pass *CompactionPass{};

  std::vector<std::unique_ptr<Pass>> Passes;

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  std::vector<std::unique_ptr<Pass>> ValidationPasses;
  void InsertValidationPass(std::unique_ptr<Pass> Pass) {
    Pass->RegisterPassManager(this);
    ValidationPasses.emplace_back(std::move(Pass));
  }
#endif

  FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);
};
}

