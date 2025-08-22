// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Utils/IntervalList.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/set.h>
#include <FEXCore/fextl/string.h>

namespace FEX::VolatileMetadata {
struct ExtendedVolatileMetadata {
  FEXCore::IntervalList<uint64_t> VolatileValidRanges;
  fextl::set<uint64_t> VolatileInstructions;
  bool ModuleTSODisabled;
};

fextl::unordered_map<fextl::string, ExtendedVolatileMetadata> ParseExtendedVolatileMetadata(std::string_view ListOfDescriptors);
} // namespace FEX::VolatileMetadata
