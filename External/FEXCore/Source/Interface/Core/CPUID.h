#pragma once
#include <functional>
#include <unordered_map>

#include <FEXCore/Core/CPUID.h>
#include <FEXCore/Utils/LogManager.h>

namespace FEXCore {
namespace Context {
  struct Context;
}

class CPUIDEmu final {
private:
  constexpr static uint32_t CPUID_VENDOR_INTEL1 = 0x756E6547; // "Genu"
  constexpr static uint32_t CPUID_VENDOR_INTEL2 = 0x49656E69; // "ineI"
  constexpr static uint32_t CPUID_VENDOR_INTEL3 = 0x6C65746E; // "ntel"

  constexpr static uint32_t CPUID_VENDOR_AMD1 = 0x68747541; // "Auth"
  constexpr static uint32_t CPUID_VENDOR_AMD2 = 0x69746E65; // "enti"
  constexpr static uint32_t CPUID_VENDOR_AMD3 = 0x444D4163; // "cAMD"

public:
  void Init(FEXCore::Context::Context *ctx);

  FEXCore::CPUID::FunctionResults RunFunction(uint32_t Function) {
    auto Handler = FunctionHandlers.find(Function);

    if (Handler == FunctionHandlers.end()) {
      #ifndef NDEBUG
        LogMan::Msg::E("Unhandled CPU ID function, 0x%x", Function);
      #endif
      return Function_Reserved();
    }

    return Handler->second();
  }
private:
  FEXCore::Context::Context *CTX;

  using FunctionHandler = std::function<FEXCore::CPUID::FunctionResults()>;
  void RegisterFunction(uint32_t Function, FunctionHandler Handler) {
    FunctionHandlers[Function] = Handler;
  }

  std::unordered_map<uint32_t, FunctionHandler> FunctionHandlers;

  // Functions
  FEXCore::CPUID::FunctionResults Function_0h();
  FEXCore::CPUID::FunctionResults Function_01h();
  FEXCore::CPUID::FunctionResults Function_02h();
  FEXCore::CPUID::FunctionResults Function_06h();
  FEXCore::CPUID::FunctionResults Function_07h();
  FEXCore::CPUID::FunctionResults Function_8000_0000h();
  FEXCore::CPUID::FunctionResults Function_8000_0001h();
  FEXCore::CPUID::FunctionResults Function_8000_0002h();
  FEXCore::CPUID::FunctionResults Function_8000_0003h();
  FEXCore::CPUID::FunctionResults Function_8000_0004h();
  FEXCore::CPUID::FunctionResults Function_8000_0005h();
  FEXCore::CPUID::FunctionResults Function_8000_0006h();
  FEXCore::CPUID::FunctionResults Function_8000_0007h();

  FEXCore::CPUID::FunctionResults Function_Reserved();
};
}
