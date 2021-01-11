#pragma once
#include <FEXCore/Config/Config.h>

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace FEXCore {
  class FD;
}
namespace FEXCore::Context {
  struct Context;
}

namespace FEX::EmulatedFile {
  class EmulatedFDManager {
    public:
      EmulatedFDManager(FEXCore::Context::Context *ctx);
      ~EmulatedFDManager();
      int32_t OpenAt(int dirfs, const char *pathname, int flags, uint32_t mode);

    private:
      FEXCore::Context::Context *CTX;
      std::string cpus_online{};
      std::string cpu_info{};
      using FDReadStringFunc = std::function<int32_t(FEXCore::Context::Context *ctx, int32_t fd, const char *pathname, int32_t flags, mode_t mode)>;
      std::unordered_map<std::string, FDReadStringFunc> FDReadCreators;

      static int32_t ProcAuxv(FEXCore::Context::Context* ctx, int32_t fd, const char* pathname, int32_t flags, mode_t mode);
      FEXCore::Config::Value<uint64_t> ThreadsConfig{FEXCore::Config::CONFIG_EMULATED_CPU_CORES, 1};
  };
}
