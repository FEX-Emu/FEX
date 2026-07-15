// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>

namespace FEX::LinuxVersion {
inline uint32_t KernelVersion(uint32_t Major, uint32_t Minor = 0, uint32_t Patch = 0) {
  return (Major << 24) | (Minor << 16) | Patch;
}

inline uint32_t KernelMajor(uint32_t Version) {
  return Version >> 24;
}
inline uint32_t KernelMinor(uint32_t Version) {
  return (Version >> 16) & 0xFF;
}

inline uint32_t KernelPatch(uint32_t Version) {
  return Version & 0xFFFF;
}

uint32_t CalculateHostKernelVersion();
} // namespace FEX::LinuxVersion
