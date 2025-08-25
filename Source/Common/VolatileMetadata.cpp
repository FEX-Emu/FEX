// SPDX-License-Identifier: MIT
#include "Common/VolatileMetadata.h"

#include <range/v3/view/split.hpp>
#include <range/v3/view/transform.hpp>

namespace FEX::VolatileMetadata {
fextl::unordered_map<fextl::string, ExtendedVolatileMetadata> ParseExtendedVolatileMetadata(std::string_view ListOfDescriptors) {
  // Parsing: `<module>;<address begin>-<address-end>,<more addresses>;<instruction offset to force TSO>:`
  if (ListOfDescriptors.empty()) {
    return {};
  }

  fextl::unordered_map<fextl::string, ExtendedVolatileMetadata> ExtendedMetaData {};

  auto to_string_view = [](auto rng) {
    return std::string_view(&*rng.begin(), ranges::distance(rng));
  };
  for (auto module_config : ranges::views::split(ListOfDescriptors, ':') | ranges::views::transform(to_string_view)) {
    if (module_config.empty()) {
      continue;
    }

    auto sections = ranges::views::split(module_config, ';') | ranges::views::transform(to_string_view);
    auto section = ranges::begin(sections);
    const auto sections_end = ranges::end(sections);

    // Module name handling
    std::string_view section_str = *section;
    if (section_str.empty()) {
      continue;
    }

    auto current_module = ExtendedMetaData
                            .insert_or_assign(fextl::string(section_str),
                                              ExtendedVolatileMetadata {
                                                .ModuleTSODisabled = true,
                                              })
                            .first;
    ++section;

    // Address range handling
    if (section != sections_end) {
      std::string_view section_str = *section;
      if (section_str.empty()) {
        continue;
      }

      current_module->second.ModuleTSODisabled = false;

      // Walk all the address ranges provided.
      for (auto tso_region_view : ranges::views::split(section_str, ',') | ranges::views::transform(to_string_view)) {
        if (tso_region_view.empty()) {
          continue;
        }

        uint64_t begin {}, end {};
        char* str_end;
        begin = std::strtoull(tso_region_view.data(), &str_end, 16);
        LOGMAN_THROW_A_FMT(tso_region_view.data() != str_end, "Couldn't parse begin {}", tso_region_view);

        // Skip `-` separator.
        ++str_end;

        LOGMAN_THROW_A_FMT(str_end != tso_region_view.end(), "Couldn't parse end {}", tso_region_view);
        auto str_begin = str_end;
        end = std::strtoull(str_begin, &str_end, 16);
        LOGMAN_THROW_A_FMT(str_begin != str_end, "Couldn't parse end {}", tso_region_view);

        current_module->second.VolatileValidRanges.Insert({begin, end});
      }

      ++section;
    }

    // Individual instruction handling
    if (section != sections_end) {
      std::string_view section_str = *section;
      if (section_str.empty()) {
        continue;
      }

      for (auto tso_region_view : ranges::views::split(section_str, ',') | ranges::views::transform(to_string_view)) {
        if (tso_region_view.empty()) {
          continue;
        }

        uint64_t offset {};
        char* str_end;
        offset = std::strtoull(tso_region_view.data(), &str_end, 16);
        LOGMAN_THROW_A_FMT(tso_region_view.data() != str_end, "Couldn't parse offset {}", tso_region_view);

        current_module->second.VolatileInstructions.insert(offset);
      }

      ++section;
    }

    LOGMAN_THROW_A_FMT(section == sections_end, "Expected ':' or end of input, got {}", *section);
  }

  return ExtendedMetaData;
}
} // namespace FEX::VolatileMetadata
