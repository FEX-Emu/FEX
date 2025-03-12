// SPDX-License-Identifier: MIT
#pragma once
#include "Common/Config.h"

namespace FEX {
static inline std::optional<fextl::string> GetSelfPath() {
  // Read the FEXInterpreter path from `/proc/self/exe` which is always a symlink to the absolute path of the executable running.
  // This way we can get the parent path that the application is executing from.
  char SelfPath[PATH_MAX];
  auto Result = readlink("/proc/self/exe", SelfPath, PATH_MAX);
  if (Result == -1) {
    return std::nullopt;
  }

  std::string_view SelfPathView {SelfPath, std::min<size_t>(PATH_MAX, Result)};
  return fextl::string {SelfPathView.substr(0, SelfPathView.find_last_of('/') + 1)};
}

static inline FEX::Config::PortableInformation ReadPortabilityInformation() {
  const char* PortableConfig = getenv("FEX_PORTABLE");
  if (!PortableConfig) {
    return {false, {}};
  }

  uint32_t Value {};
  std::string_view PortableView {PortableConfig};

  if (std::from_chars(PortableView.data(), PortableView.data() + PortableView.size(), Value).ec != std::errc {} || Value == 0) {
    return {false, {}};
  }

  auto SelfPath = GetSelfPath();
  if (!SelfPath) {
    return {false, {}};
  }

  // Extract the absolute path from the FEXInterpreter path
  return {true, *SelfPath};
}
} // namespace FEX
