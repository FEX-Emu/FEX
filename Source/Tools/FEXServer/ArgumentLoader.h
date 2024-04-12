// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>

namespace FEXServer::Config {
struct FEXServerOptions {
  bool Kill;
  bool Foreground;
  bool Wait;
  uint32_t PersistentTimeout;
};

FEXServerOptions Load(int argc, char** argv);
} // namespace FEXServer::Config
