#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string_view>

namespace FEXCore {
class JITSymbols final {
public:
  JITSymbols();
  ~JITSymbols();

  void InitFile();
  void Register(const void *HostAddr, uint64_t GuestAddr, uint32_t CodeSize);
  void Register(const void *HostAddr, uint32_t CodeSize, std::string_view Name);
  void Register(const void *HostAddr, uint32_t CodeSize, std::string_view Name, uintptr_t Offset);
  void RegisterNamedRegion(const void *HostAddr, uint32_t CodeSize, std::string_view Name);
  void RegisterJITSpace(const void *HostAddr, uint32_t CodeSize);

private:
  int fd{-1};
};
}
