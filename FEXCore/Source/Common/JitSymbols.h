#pragma once

#include <FEXCore/fextl/memory.h>
#include <FEXCore/Debug/InternalThreadState.h>

#include <chrono>
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
  void RegisterNamedRegion(const void *HostAddr, uint32_t CodeSize, std::string_view Name);
  void RegisterJITSpace(const void *HostAddr, uint32_t CodeSize);

  // Allocate JIT buffer.
  static fextl::unique_ptr<Core::JITSymbolBuffer> AllocateBuffer() {
    return fextl::make_unique<Core::JITSymbolBuffer>();
  }

  void Register(Core::JITSymbolBuffer *Buffer, const void *HostAddr, uint64_t GuestAddr, uint32_t CodeSize);
  void Register(Core::JITSymbolBuffer *Buffer, const void *HostAddr, uint32_t CodeSize, std::string_view Name, uintptr_t Offset);
  void RegisterNamedRegion(Core::JITSymbolBuffer *Buffer, const void *HostAddr, uint32_t CodeSize, std::string_view Name);

private:
  int fd{-1};
  void WriteBuffer(Core::JITSymbolBuffer *Buffer, bool ForceWrite = false);
};
}
