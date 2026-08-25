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
  enum class HostTypeEnum {
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

  [[nodiscard]]
  uint64_t HashForCaching() const {
    // As long as the number of options is 64-bit or below, we can just return it.
    // Skip CPUMIDRs as it doesn't affect codegen.
    static_assert(offsetof(HostFeatures, CPUMIDRs) == 8);
    uint64_t Result {};
    memcpy(&Result, this, sizeof(Result));
    return Result;
  }

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
  uint32_t PreferZVAForVZero           : 1 {};
  uint32_t SupportsAFP                 : 1 {};
  uint32_t SupportsFloatExceptions     : 1 {};
  // Flag if this is InstCountCI
  uint32_t IsInstCountCI : 1 {};
  HostTypeEnum HostType  : 2 {};
  uint32_t pad           : 26 {};

  // MIDR information
  // Also used for determining number of CPU cores for CPUID
  fextl::vector<uint32_t> CPUMIDRs;
};
} // namespace FEXCore
