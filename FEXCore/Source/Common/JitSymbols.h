// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Utils/TypeDefines.h>

#include <FEXCore/fextl/memory.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace FEXCore {
// Buffered JIT symbol tracking.
struct JITSymbolBuffer {
  // Maximum buffer size to ensure we are a page in size.
  constexpr static size_t BUFFER_SIZE = FEXCore::Utils::FEX_PAGE_SIZE - (8 * 2);
  // Maximum distance until the end of the buffer to do a write.
  constexpr static size_t NEEDS_WRITE_DISTANCE = BUFFER_SIZE - 64;
  // Maximum time threshhold to wait before a buffer write occurs.
  constexpr static std::chrono::milliseconds MAXIMUM_THRESHOLD {100};

  JITSymbolBuffer()
    : LastWrite {std::chrono::steady_clock::now()} {}
  // stead_clock to ensure a monotonic increasing clock.
  // In highly stressed situations this can still cause >2% CPU time in vdso_clock_gettime.
  // If we need lower CPU time when JIT symbols are enabled then FEX can read the cycle counter directly.
  std::chrono::steady_clock::time_point LastWrite {};
  size_t Offset {};
  char Buffer[BUFFER_SIZE] {};
};
static_assert(sizeof(JITSymbolBuffer) == FEXCore::Utils::FEX_PAGE_SIZE, "Ensure this is one page in size");

class JITSymbols final {
public:
  JITSymbols();
  ~JITSymbols();

  void InitFile();
  void RegisterNamedRegion(const void* HostAddr, uint32_t CodeSize, std::string_view Name);
  void RegisterJITSpace(const void* HostAddr, uint32_t CodeSize);

  // Allocate JIT buffer.
  static fextl::unique_ptr<FEXCore::JITSymbolBuffer> AllocateBuffer() {
    return fextl::make_unique<FEXCore::JITSymbolBuffer>();
  }

  void Register(FEXCore::JITSymbolBuffer* Buffer, const void* HostAddr, uint64_t GuestAddr, uint32_t CodeSize);
  void Register(FEXCore::JITSymbolBuffer* Buffer, const void* HostAddr, uint32_t CodeSize, std::string_view Name, uintptr_t Offset);
  void RegisterNamedRegion(FEXCore::JITSymbolBuffer* Buffer, const void* HostAddr, uint32_t CodeSize, std::string_view Name);

private:
  int fd {-1};
  void WriteBuffer(FEXCore::JITSymbolBuffer* Buffer, bool ForceWrite = false);
};
} // namespace FEXCore
