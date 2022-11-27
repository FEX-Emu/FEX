/*
$info$
tags: LinuxSyscalls|common
$end_info$
*/

#pragma once
#include <FEXCore/Config/Config.h>

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <string>
#include <sys/types.h>

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
      std::once_flag cpu_info_initialized{};
      std::string cpu_info{};
      using FDReadStringFunc = std::function<int32_t(FEXCore::Context::Context *ctx, int32_t fd, const char *pathname, int32_t flags, mode_t mode)>;
      std::unordered_map<std::string, FDReadStringFunc> FDReadCreators;

      static int32_t ProcAuxv(FEXCore::Context::Context* ctx, int32_t fd, const char* pathname, int32_t flags, mode_t mode);
      FEX_CONFIG_OPT(ThreadsConfig, THREADS);
  };
}
