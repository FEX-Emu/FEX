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

#include <concepts>
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

  // Executes all of the passes added to the manager.
  // If assertions are enabled, this will also run all validation passes.
  void Run(IREmitter* IREmit);

  // Inserts a new pass into the manager, optionally also assigning a name to it
  // for use in the lookup functions,
  Pass* InsertPass(fextl::unique_ptr<Pass> Pass, const fextl::string& Name = "");

  // Whether or not a pass with the given name is within the manager.
  bool HasPass(const fextl::string& Name) const {
    return NameToPassMaping.contains(Name);
  }

  // Retrieves a pass from the manager that has the given name assigned to it.
  // Will return nullptr if the pass doesn't exist.
  template<std::derived_from<Pass> T>
  T* GetPass(const fextl::string& Name) {
    return dynamic_cast<T*>(GetPass(Name));
  }
  Pass* GetPass(const fextl::string& Name) {
    return NameToPassMaping[Name];
  }

  // Finalizes the pass manager state and assumes no other passes will be added after called.
  // This will reorganize the execution order of the passes if necessary.
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
