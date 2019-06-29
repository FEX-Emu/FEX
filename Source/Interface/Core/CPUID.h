#pragma once
#include <functional>
#include <unordered_map>

#include "LogManager.h"

namespace FEXCore {

class CPUIDEmu final {
public:
  void Init();

  struct FunctionResults {
    // Results in registers EAX, EBX, EDX, ECX respectively
    uint32_t Res[4];
  };

  FunctionResults RunFunction(uint32_t Function) {
    LogMan::Throw::A(FunctionHandlers.find(Function) != FunctionHandlers.end(), "Don't have a CPUID handler for 0x%08x", Function);
    return FunctionHandlers[Function]();
  }
private:

  using FunctionHandler = std::function<FunctionResults()>;
  void RegisterFunction(uint32_t Function, FunctionHandler Handler) {
    FunctionHandlers[Function] = Handler;
  }

  std::unordered_map<uint32_t, FunctionHandler> FunctionHandlers;

  // Functions
  FunctionResults Function_0h();
  FunctionResults Function_01h();
  FunctionResults Function_02h();
  FunctionResults Function_04h();
  FunctionResults Function_07h();
  FunctionResults Function_0Dh();
  FunctionResults Function_8000_0000h();
  FunctionResults Function_8000_0001h();
  FunctionResults Function_8000_0006h();
  FunctionResults Function_8000_0007h();
  FunctionResults Function_8000_0008h();
};
}
