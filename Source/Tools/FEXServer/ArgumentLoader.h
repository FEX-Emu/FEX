#pragma once
#include <cstdint>

namespace FEXServer::Config {
  struct FEXServerOptions {
    bool Kill;
    bool Foreground;
    uint32_t PersistentTimeout;
  };

  FEXServerOptions Load(int argc, char **argv);
}
