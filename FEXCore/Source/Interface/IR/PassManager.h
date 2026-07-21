// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
$end_info$
*/

#pragma once

#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/vector.h>

#include <utility>

namespace FEXCore::Context {
class ContextImpl;
}

namespace FEXCore::IR {
class PassManager;
class IREmitter;

class Pass {
public:
  virtual ~Pass() = default;
  virtual void Run(IREmitter* IREmit) = 0;

  void RegisterPassManager(PassManager* _Manager) {
    Manager = _Manager;
  }

protected:
  PassManager* Manager {};
};

class PassManager final {
public:
  explicit PassManager(Context::ContextImpl* CTX) {
    AddDefaultPasses(CTX);
  }

  void Run(IREmitter* IREmit);

  Pass* InsertPass(fextl::unique_ptr<Pass> Pass, const fextl::string& Name = "");

  bool HasPass(const fextl::string& Name) const {
    return NameToPassMaping.contains(Name);
  }

  template<typename T>
  T* GetPass(const fextl::string& Name) {
    return dynamic_cast<T*>(GetPass(Name));
  }

  Pass* GetPass(const fextl::string& Name) {
    return NameToPassMaping[Name];
  }

  void Finalize();

private:
  void AddDefaultPasses(Context::ContextImpl* ctx);

  using PassArrayType = fextl::vector<fextl::unique_ptr<Pass>>;
  PassArrayType::iterator InsertAt(PassArrayType::iterator pos, fextl::unique_ptr<Pass> Pass);

  PassArrayType Passes;
  fextl::unordered_map<fextl::string, Pass*> NameToPassMaping;

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  fextl::vector<fextl::unique_ptr<Pass>> ValidationPasses;
  void InsertValidationPass(fextl::unique_ptr<Pass> Pass, const fextl::string& Name = "");
#endif

  void AttemptNameMapping(const fextl::string& Name, Pass* NewPass);

  FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);
  FEX_CONFIG_OPT(PassManagerDumpIR, PASSMANAGERDUMPIR);
};
} // namespace FEXCore::IR
