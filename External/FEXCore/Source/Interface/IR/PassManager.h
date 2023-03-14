/*
$info$
tags: ir|opts
$end_info$
*/

#pragma once

#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/ThreadPoolAllocator.h>
#include <FEXCore/fextl/vector.h>

#include <functional>
#include <memory>
#include <utility>

namespace FEXCore::Context {
  class ContextImpl;
}

namespace FEXCore::HLE {
class SyscallHandler;
}

namespace FEXCore::IR {
class PassManager;
class IREmitter;

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
  void AddDefaultPasses(FEXCore::Context::ContextImpl *ctx, bool InlineConstants, bool StaticRegisterAllocation);
  void AddDefaultValidationPasses();
  Pass* InsertPass(std::unique_ptr<Pass> Pass, std::string Name = "") {
    Pass->RegisterPassManager(this);
    auto PassPtr = Passes.emplace_back(std::move(Pass)).get();

    if (!Name.empty()) {
      NameToPassMaping[Name] = PassPtr;
    }
    return PassPtr;
  }

  void InsertRegisterAllocationPass(bool OptimizeSRA, bool SupportsAVX);

  bool Run(IREmitter *IREmit);

  void RegisterExitHandler(ShouldExitHandler Handler) {
    ExitHandler = std::move(Handler);
  }

  bool HasPass(std::string Name) const {
    return NameToPassMaping.contains(Name);
  }

  template<typename T>
  T* GetPass(std::string Name) {
    return dynamic_cast<T*>(NameToPassMaping[Name]);
  }

  Pass* GetPass(std::string Name) {
    return NameToPassMaping[Name];
  }

  void RegisterSyscallHandler(FEXCore::HLE::SyscallHandler *Handler) {
    SyscallHandler = Handler;
  }

protected:
  ShouldExitHandler ExitHandler;
  FEXCore::HLE::SyscallHandler *SyscallHandler;

private:
  fextl::vector<std::unique_ptr<Pass>> Passes;
  std::unordered_map<std::string, Pass*> NameToPassMaping;

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  fextl::vector<std::unique_ptr<Pass>> ValidationPasses;
  void InsertValidationPass(std::unique_ptr<Pass> Pass, std::string Name = "") {
    Pass->RegisterPassManager(this);
    auto PassPtr = ValidationPasses.emplace_back(std::move(Pass)).get();

    if (!Name.empty()) {
      NameToPassMaping[Name] = PassPtr;
    }
  }
#endif

  FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);
};
}

