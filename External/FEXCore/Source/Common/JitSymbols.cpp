#include "Common/JitSymbols.h"

#include <fcntl.h>
#include <string>
#include <unistd.h>

#include <fmt/format.h>

namespace FEXCore {
  JITSymbols::JITSymbols() {
  }

  JITSymbols::~JITSymbols() {
    if (fd != -1) {
      close(fd);
    }
  }

  void JITSymbols::InitFile() {
    // We can't use FILE here since we must be robust against forking processes closing our FD from under us.
    const auto PerfMap = fmt::format("/tmp/perf-{}.map", getpid());

    fd = open(PerfMap.c_str(), O_CREAT | O_TRUNC | O_WRONLY | O_APPEND, 0644);
  }

  void JITSymbols::Register(const void *HostAddr, uint64_t GuestAddr, uint32_t CodeSize) {
    if (fd == -1) return;

    // Linux perf format is very straightforward
    // `<HostPtr> <Size> <Name>\n`
    const auto Buffer = fmt::format("{} {:x} JIT_0x{:x}_{}\n", HostAddr, CodeSize, GuestAddr, HostAddr);
    auto Result = write(fd, Buffer.c_str(), Buffer.size());
    if (Result == -1 && errno == EBADF) {
      fd = -1;
    }
  }

  void JITSymbols::Register(const void *HostAddr, uint32_t CodeSize, std::string_view Name) {
    if (fd == -1) return;

    // Linux perf format is very straightforward
    // `<HostPtr> <Size> <Name>\n`
    const auto Buffer = fmt::format("{} {:x} {}_{}\n", HostAddr, CodeSize, Name, HostAddr);
    auto Result = write(fd, Buffer.c_str(), Buffer.size());
    if (Result == -1 && errno == EBADF) {
      fd = -1;
    }
  }

  void JITSymbols::Register(const void *HostAddr, uint32_t CodeSize, std::string_view Name, uintptr_t Offset) {
    if (fd == -1) return;

    // Linux perf format is very straightforward
    // `<HostPtr> <Size> <Name>\n`
    const auto Buffer = fmt::format("{} {:x} {}+0x{:x} ({})\n", HostAddr, CodeSize, Name, Offset, HostAddr);
    auto Result = write(fd, Buffer.c_str(), Buffer.size());
    if (Result == -1 && errno == EBADF) {
      fd = -1;
    }
  }

  void JITSymbols::RegisterNamedRegion(const void *HostAddr, uint32_t CodeSize, std::string_view Name) {
    if (fd == -1) return;

    // Linux perf format is very straightforward
    // `<HostPtr> <Size> <Name>\n`
    const auto Buffer = fmt::format("{} {:x} {}\n", HostAddr, CodeSize, Name);
    auto Result = write(fd, Buffer.c_str(), Buffer.size());
    if (Result == -1 && errno == EBADF) {
      fd = -1;
    }
  }

  void JITSymbols::RegisterJITSpace(const void *HostAddr, uint32_t CodeSize) {
    if (fd == -1) return;

    // Linux perf format is very straightforward
    // `<HostPtr> <Size> <Name>\n`
    const auto Buffer = fmt::format("{} {:x} FEXJIT\n", HostAddr, CodeSize);
    auto Result = write(fd, Buffer.c_str(), Buffer.size());
    if (Result == -1 && errno == EBADF) {
      fd = -1;
    }
  }

} // namespace FEXCore
