#pragma once

#include <FEXCore/IR/IntrusiveIRList.h>

#include <memory>
#include <vector>

namespace FEXCore::IR {
class OpDispatchBuilder;

class Pass {
public:
  virtual ~Pass() = default;
  virtual bool Run(OpDispatchBuilder *Disp) = 0;
};

class PassManager final {
public:
  void AddDefaultPasses();
  void AddDefaultValidationPasses();
  bool Run(OpDispatchBuilder *Disp);

private:
  std::vector<std::unique_ptr<Pass>> Passes;
};
}

