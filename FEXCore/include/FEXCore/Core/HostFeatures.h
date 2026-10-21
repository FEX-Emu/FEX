// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/vector.h>
#include <cstdint>

namespace FEXCore {

/**
 * @brief Backend features that change how codegen is generated from IR
 *
 * Specifically things that affect the IR->Codegen process
 * Not the x86->IR process
 */
struct HostFeatures {
  // Changes code generation slightly.
  enum class HostTypeEnum : uint8_t {
    Unknown,
    Linux,
    Wow64,
    Arm64ec,
  };

  // Whether or not the host supports any kind of SVE implementation.
  [[nodiscard]]
  bool SupportsSVE() const {
    return SupportsSVE128 || SupportsSVE256;
  }

  [[nodiscard]]
  uint32_t DCacheSize() const {
    return 4 << DCacheLineLog2;
  }

  /**
   * @brief This structure contains all the data required to determine hashing at the "process" level.
   * Unlike the `HostFeaturesHash` below, this data changes codegen depending on if the process is Linux, Arm64EC, or Wow64.
   */
  struct ProcessFeaturesHash {
    HostTypeEnum HostType                      : 2 {};
    uint8_t SupportsFaultFreeSynchronousFaults : 1 {};
    // This only determines the starting mode a process started in.
    // The process can technically long jump to-from the other modes.
    uint8_t Is64BitMode : 1 {};
    uint8_t pad         : 4 {};
  };
  static_assert(sizeof(ProcessFeaturesHash) == 1);

  struct CacheHash {
    // This hash is used to determine the "bucket" a machine goes in to.
    // This should match between Linux, Arm64EC, and Wow64.
    uint64_t HostFeaturesHash;

    // This hash is used to determine features that are different between process types.
    ProcessFeaturesHash ProcessFeaturesHash;
  };

  [[nodiscard]]
  CacheHash HashForCaching() const {
    static_assert(offsetof(HostFeatures, ProcessFeatures) == 8);

    CacheHash Result {};
    memcpy(&Result.HostFeaturesHash, this, sizeof(uint64_t));
    Result.ProcessFeaturesHash = ProcessFeatures;

    return Result;
  }

  // Affects codegen and is basically machine description.
  uint32_t DCacheLineLog2              : 4 {};
  uint32_t SupportsCacheMaintenanceOps : 1 {};
  uint32_t SupportsAES                 : 1 {};
  uint32_t SupportsCRC                 : 1 {};
  uint32_t SupportsCLZERO              : 1 {};
  uint32_t SupportsAtomics             : 1 {};
  uint32_t SupportsRCPC                : 1 {};
  uint32_t SupportsTSOImm9             : 1 {};
  uint32_t SupportsRAND                : 1 {};
  uint32_t SupportsAVX                 : 1 {};
  uint32_t SupportsSVE128              : 1 {};
  uint32_t SupportsSVE256              : 1 {};
  uint32_t SupportsSHA                 : 1 {};
  uint32_t SupportsPMULL_128Bit        : 1 {};
  uint32_t SupportsCSSC                : 1 {};
  uint32_t SupportsFCMA                : 1 {};
  uint32_t SupportsFlagM               : 1 {};
  uint32_t SupportsFlagM2              : 1 {};
  uint32_t SupportsRPRES               : 1 {};
  uint32_t SupportsPreserveAllABI      : 1 {};
  uint32_t SupportsAES256              : 1 {};
  uint32_t SupportsSVEBitPerm          : 1 {};
  uint32_t SupportsCPUIndexInTPIDRRO   : 1 {};
  uint32_t SupportsFRINTTS             : 1 {};
  uint32_t SupportsECV                 : 1 {};
  uint32_t SupportsWFXT                : 1 {};
  uint32_t Supports3DNow               : 1 {};
  uint32_t SupportsSSE4a               : 1 {};
  uint32_t SupportsMOPS                : 1 {};
  uint32_t SupportsI8MM                : 1 {};
  uint32_t SupportsDotProd             : 1 {};
  uint32_t PreferZVAForVZero           : 1 {};
  uint32_t SupportsAFP                 : 1 {};
  uint32_t SupportsFloatExceptions     : 1 {};
  uint32_t SupportsBF16                : 1 {};
  // Flag if this is InstCountCI
  uint32_t IsInstCountCI : 1 {};
  uint32_t pad           : 25 {};

  // This affects codegen, but it isn't machine state
  ProcessFeaturesHash ProcessFeatures {};

  // MIDR information
  // Also used for determining number of CPU cores for CPUID
  fextl::vector<uint32_t> CPUMIDRs;

  // The Linux PID of this process. Useful for punching through perf-top information.
  uint32_t ProcessPID {};
  uint32_t pad2 {};
};
} // namespace FEXCore
