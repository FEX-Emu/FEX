// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|common
$end_info$
*/

#pragma once
#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/string.h>

#include <cstdint>
#include <functional>
#include <sys/types.h>

namespace FEXCore::Context {
class Context;
}

namespace FEX::EmulatedFile {
class EmulatedFDManager {
public:
  EmulatedFDManager(FEXCore::Context::Context* ctx);
  ~EmulatedFDManager();
  int32_t Open(const char* pathname, int flags, uint32_t mode);

private:
  FEXCore::Context::Context* CTX;
  fextl::string cpus_online {};
  std::once_flag cpu_info_initialized {};
  fextl::string cpu_info {};
  using FDReadStringFunc = std::function<int32_t(FEXCore::Context::Context* ctx, int32_t fd, const char* pathname, int32_t flags, mode_t mode)>;
  fextl::unordered_map<fextl::string, FDReadStringFunc> FDReadCreators;

  static int32_t ProcAuxv(FEXCore::Context::Context* ctx, int32_t fd, const char* pathname, int32_t flags, mode_t mode);
  const uint32_t ThreadsConfig;
};
} // namespace FEX::EmulatedFile
