#include "Common/JitSymbols.h"

#include <string>
#include <unistd.h>

#include <fmt/format.h>

namespace FEXCore {
  JITSymbols::JITSymbols() : fp{nullptr, std::fclose} {
    const auto PerfMap = fmt::format("/tmp/perf-{}.map", getpid());

    fp.reset(fopen(PerfMap.c_str(), "wb"));
    if (fp) {
      // Disable buffering on this file
      setvbuf(fp.get(), nullptr, _IONBF, 0);
    }
  }

  JITSymbols::~JITSymbols() = default;

  void JITSymbols::Register(const void *HostAddr, uint64_t GuestAddr, uint32_t CodeSize) {
    if (!fp) return;

    // Linux perf format is very straightforward
    // `<HostPtr> <Size> <Name>\n`
    fmt::print(fp.get(), "{:x} {:x} JIT_0x{:x}_{:x}\n", HostAddr, CodeSize, GuestAddr, HostAddr);
  }

  void JITSymbols::Register(const void *HostAddr, uint32_t CodeSize, std::string const &Name) {
    if (!fp) return;

    // Linux perf format is very straightforward
    // `<HostPtr> <Size> <Name>\n`
    fmt::print(fp.get(), "{:x} {:x} {}_{:x}\n", HostAddr, CodeSize, Name, HostAddr);
  }

  void JITSymbols::RegisterNamedRegion(const void *HostAddr, uint32_t CodeSize, std::string const &Name) {
    if (!fp) return;

    // Linux perf format is very straightforward
    // `<HostPtr> <Size> <Name>\n`
    fmt::print(fp.get(), "{:x} {:x} {}\n", HostAddr, CodeSize, Name);
  }

  void JITSymbols::RegisterJITSpace(const void *HostAddr, uint32_t CodeSize) {
    if (!fp) return;

    // Linux perf format is very straightforward
    // `<HostPtr> <Size> <Name>\n`
    fmt::print(fp.get(), "{:x} {:x} FEXJIT\n", HostAddr, CodeSize);
  }

} // namespace FEXCore
