#pragma once
#include <cstdint>
#include <cstdio>

namespace FEXCore {
class JITSymbols final {
public:
  JITSymbols();
  ~JITSymbols();
  void Register(void *HostAddr, uint64_t GuestAddr, uint32_t CodeSize);
private:
  FILE* fp{};
};
}
