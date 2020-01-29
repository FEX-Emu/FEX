#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace FEXCore {
  class FD;
}
namespace FEXCore::Context {
  struct Context;
}

namespace FEXCore::EmulatedFile {
  class EmulatedFDManager {
    public:
      EmulatedFDManager(FEXCore::Context::Context *ctx);
      ~EmulatedFDManager();
      FEXCore::FD *OpenAt(int dirfs, const char *pathname, int flags, uint32_t mode);

    private:
      FEXCore::Context::Context *CTX;
      using FDReadStringFunc = std::function<FEXCore::FD*(FEXCore::Context::Context *ctx, int32_t fd, const char *pathname, int32_t flags, mode_t mode)>;
      std::unordered_set<std::string> EmulatedMap;
      std::unordered_map<std::string, FDReadStringFunc> FDReadCreators;
  };
}
