// SPDX-License-Identifier: MIT
#include <FEXCore/fextl/fmt.h>

#include "Common/JitSymbols.h"

#include <fcntl.h>
#include <unistd.h>

namespace FEXCore {
JITSymbols::JITSymbols() {}

JITSymbols::~JITSymbols() {
  if (fd != -1) {
    close(fd);
  }
}

void JITSymbols::InitFile() {
  // We can't use FILE here since we must be robust against forking processes closing our FD from under us.
#ifdef __ANDROID__
  // Android simpleperf looks in /data/local/tmp instead of /tmp
  const auto PerfMap = fextl::fmt::format("/data/local/tmp/perf-{}.map", getpid());
#else
  const auto PerfMap = fextl::fmt::format("/tmp/perf-{}.map", getpid());
#endif
  fd = open(PerfMap.c_str(), O_CREAT | O_TRUNC | O_WRONLY | O_APPEND, 0644);
}

void JITSymbols::RegisterNamedRegion(const void* HostAddr, uint32_t CodeSize, std::string_view Name) {
  if (fd == -1) {
    return;
  }

  // Linux perf format is very straightforward
  // `<HostPtr> <Size> <Name>\n`
  const auto Buffer = fextl::fmt::format("{} {:x} {}\n", HostAddr, CodeSize, Name);
  auto Result = write(fd, Buffer.c_str(), Buffer.size());
  if (Result == -1 && errno == EBADF) {
    fd = -1;
  }
}

void JITSymbols::RegisterJITSpace(const void* HostAddr, uint32_t CodeSize) {
  if (fd == -1) {
    return;
  }

  // Linux perf format is very straightforward
  // `<HostPtr> <Size> <Name>\n`
  const auto Buffer = fextl::fmt::format("{} {:x} FEXJIT\n", HostAddr, CodeSize);
  auto Result = write(fd, Buffer.c_str(), Buffer.size());
  if (Result == -1 && errno == EBADF) {
    fd = -1;
  }
}

// Buffered JIT symbols.
void JITSymbols::Register(FEXCore::JITSymbolBuffer* Buffer, const void* HostAddr, uint64_t GuestAddr, uint32_t CodeSize) {
  if (fd == -1) {
    return;
  }

  // Calculate remaining sizes.
  const auto RemainingSize = Buffer->BUFFER_SIZE - Buffer->Offset;
  const auto CurrentBufferOffset = &Buffer->Buffer[Buffer->Offset];

  // Linux perf format is very straightforward
  // `<HostPtr> <Size> <Name>\n`
  const auto FMTResult = fmt::format_to_n(CurrentBufferOffset, RemainingSize, "{} {:x} JIT_0x{:x}_{}\n", HostAddr, CodeSize, GuestAddr, HostAddr);
  if (FMTResult.out >= &Buffer->Buffer[Buffer->BUFFER_SIZE]) {
    // Couldn't fit, need to force a write.
    WriteBuffer(Buffer, true);
    // Rerun
    Register(Buffer, HostAddr, GuestAddr, CodeSize);
    return;
  }

  Buffer->Offset += FMTResult.size;
  WriteBuffer(Buffer);
}

void JITSymbols::Register(FEXCore::JITSymbolBuffer* Buffer, const void* HostAddr, uint32_t CodeSize, std::string_view Name, uintptr_t Offset) {
  if (fd == -1) {
    return;
  }

  // Calculate remaining sizes.
  const auto RemainingSize = Buffer->BUFFER_SIZE - Buffer->Offset;
  const auto CurrentBufferOffset = &Buffer->Buffer[Buffer->Offset];

  // Linux perf format is very straightforward
  // `<HostPtr> <Size> <Name>\n`
  const auto FMTResult =
    fmt::format_to_n(CurrentBufferOffset, RemainingSize, "{} {:x} {}+0x{:x} ({})\n", HostAddr, CodeSize, Name, Offset, HostAddr);
  if (FMTResult.out >= &Buffer->Buffer[Buffer->BUFFER_SIZE]) {
    // Couldn't fit, need to force a write.
    WriteBuffer(Buffer, true);
    // Rerun
    Register(Buffer, HostAddr, CodeSize, Name, Offset);
    return;
  }

  Buffer->Offset += FMTResult.size;
  WriteBuffer(Buffer);
}

void JITSymbols::RegisterNamedRegion(FEXCore::JITSymbolBuffer* Buffer, const void* HostAddr, uint32_t CodeSize, std::string_view Name) {
  if (fd == -1) {
    return;
  }

  // Calculate remaining sizes.
  const auto RemainingSize = Buffer->BUFFER_SIZE - Buffer->Offset;
  const auto CurrentBufferOffset = &Buffer->Buffer[Buffer->Offset];

  // Linux perf format is very straightforward
  // `<HostPtr> <Size> <Name>\n`
  const auto FMTResult = fmt::format_to_n(CurrentBufferOffset, RemainingSize, "{} {:x} {}\n", HostAddr, CodeSize, Name);
  if (FMTResult.out >= &Buffer->Buffer[Buffer->BUFFER_SIZE]) {
    // Couldn't fit, need to force a write.
    WriteBuffer(Buffer, true);
    // Rerun
    RegisterNamedRegion(Buffer, HostAddr, CodeSize, Name);
    return;
  }

  Buffer->Offset += FMTResult.size;
  WriteBuffer(Buffer);
}

void JITSymbols::WriteBuffer(FEXCore::JITSymbolBuffer* Buffer, bool ForceWrite) {
  auto Now = std::chrono::steady_clock::now();
  if (!ForceWrite) {
    if (((Buffer->LastWrite - Now) < Buffer->MAXIMUM_THRESHOLD) && Buffer->Offset < Buffer->NEEDS_WRITE_DISTANCE) {
      // Still buffering, no need to write.
      return;
    }
  }

  Buffer->LastWrite = Now;
  auto Result = write(fd, Buffer->Buffer, Buffer->Offset);
  if (Result == -1 && errno == EBADF) {
    fd = -1;
  }

  Buffer->Offset = 0;
}
} // namespace FEXCore
