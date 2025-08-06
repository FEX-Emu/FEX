// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/vector.h>
#include <cstdint>

namespace FEXCore {
struct HostFeatures {
  /**
   * @brief Backend features that change how codegen is generated from IR
   *
   * Specifically things that affect the IR->Codegen process
   * Not the x86->IR process
   */
  uint32_t DCacheLineSize {};
  uint32_t ICacheLineSize {};
  bool SupportsCacheMaintenanceOps {};
  bool SupportsAES {};
  bool SupportsCRC {};
  bool SupportsCLZERO {};
  bool SupportsAtomics {};
  bool SupportsRCPC {};
  bool SupportsTSOImm9 {};
  bool SupportsRAND {};
  bool SupportsAVX {};
  bool SupportsSVE128 {};
  bool SupportsSVE256 {};
  bool SupportsSHA {};
  bool SupportsPMULL_128Bit {};
  bool SupportsCSSC {};
  bool SupportsFCMA {};
  bool SupportsFlagM {};
  bool SupportsFlagM2 {};
  bool SupportsRPRES {};
  bool SupportsPreserveAllABI {};
  bool SupportsAES256 {};
  bool SupportsSVEBitPerm {};
  bool SupportsCPUIndexInTPIDRRO {};
  bool SupportsFRINTTS {};
  bool SupportsECV {};
  bool SupportsWFXT {};
  bool Supports3DNow {};

  // Float exception behaviour
  bool SupportsAFP {};
  bool SupportsFloatExceptions {};

  // Flag if this is InstCountCI
  bool IsInstCountCI {};

  // MIDR information
  // Also used for determining number of CPU cores for CPUID
  fextl::vector<uint32_t> CPUMIDRs;
};
} // namespace FEXCore
