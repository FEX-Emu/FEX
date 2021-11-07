#pragma once
#include <functional>
#include <unordered_map>

#include <FEXCore/Core/CPUID.h>
#include <FEXCore/Config/Config.h>

#include <cstdint>
#include <utility>

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

  FEXCore::CPUID::FunctionResults RunFunction(uint32_t Function, uint32_t Leaf) {
    auto Handler = FunctionHandlers.find(Function);

    if (Handler == FunctionHandlers.end()) {
      return Function_Reserved(Leaf);
    }

    return Handler->second(Leaf);
  }
private:
  FEXCore::Context::Context *CTX;
  bool Hybrid{};
  FEX_CONFIG_OPT(Cores, THREADS);

  using FunctionHandler = std::function<FEXCore::CPUID::FunctionResults(uint32_t Leaf)>;
  void RegisterFunction(uint32_t Function, FunctionHandler Handler) {
    FunctionHandlers[Function] = Handler;
  }

  std::unordered_map<uint32_t, FunctionHandler> FunctionHandlers;

  // Functions
  FEXCore::CPUID::FunctionResults Function_0h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_01h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_02h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_04h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_06h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_07h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_0Dh(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_15h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0000h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0001h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0002h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0003h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0004h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0005h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0006h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0007h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0008h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0009h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_0019h(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_8000_001Dh(uint32_t Leaf);
  FEXCore::CPUID::FunctionResults Function_Reserved(uint32_t Leaf);
};
}
