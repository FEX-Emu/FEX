// SPDX-License-Identifier: MIT
#include <cstdint>
#include <charconv>
#include <sys/utsname.h>

namespace FEX::LinuxVersion {
uint32_t CalculateHostKernelVersion() {
  struct utsname buf {};
  if (uname(&buf) == -1) {
    return 0;
  }

  uint32_t Major {};
  uint32_t Minor {};
  uint32_t Patch {};

  // Parse kernel version in the form of `<Major>.<Minor>.<Patch>[Optional Data]`
  const auto End = buf.release + sizeof(buf.release);
  auto Results = std::from_chars(buf.release, End, Major, 10);
  Results = std::from_chars(Results.ptr + 1, End, Minor, 10);
  Results = std::from_chars(Results.ptr + 1, End, Patch, 10);

  return (Major << 24) | (Minor << 16) | Patch;
}
} // namespace FEX::LinuxVersion
