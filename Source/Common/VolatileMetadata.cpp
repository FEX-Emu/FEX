// SPDX-License-Identifier: MIT
#include "Common/VolatileMetadata.h"

namespace FEX::VolatileMetadata {
fextl::unordered_map<fextl::string, ExtendedVolatileMetadata> ParseExtendedVolatileMetadata(std::string_view ListOfDescriptors) {
  // Parsing: `<module>;<address begin>-<address-end>,<more addresses>;<instruction offset to force TSO>:`
  if (ListOfDescriptors.empty()) {
    return {};
  }

  fextl::unordered_map<fextl::string, ExtendedVolatileMetadata> ExtendedMetaData {};

  auto current_module = ExtendedMetaData.end();

  for (size_t module_offset = 0; module_offset != ListOfDescriptors.npos;) {
    size_t end_of_module = ListOfDescriptors.find(":", module_offset);
    std::string_view module_config = ListOfDescriptors.substr(module_offset, end_of_module - module_offset);

    if (module_config.empty()) {
      module_offset = end_of_module == ListOfDescriptors.npos ? ListOfDescriptors.npos : end_of_module + 1;
      continue;
    }

    size_t end_of_name = module_config.find(";");
    size_t end_of_address_ranges = module_config.npos;
    size_t end_of_individual_inst = module_config.npos;

    // Module name handling
    {
      std::string_view section_str = module_config.substr(0, end_of_name);

      if (section_str.empty()) {
        module_offset = end_of_module == ListOfDescriptors.npos ? ListOfDescriptors.npos : end_of_module + 1;
        continue;
      }

      current_module = ExtendedMetaData
                         .insert_or_assign(fextl::string(section_str),
                                           ExtendedVolatileMetadata {
                                             .ModuleTSODisabled = true,
                                           })
                         .first;
    }

    // Address range handling
    if (end_of_name != module_config.npos) {
      end_of_address_ranges = module_config.find(";", end_of_name + 1);
      std::string_view section_str = module_config.substr(end_of_name + 1, end_of_address_ranges - (end_of_name + 1));

      if (section_str.empty()) {
        module_offset = end_of_module == ListOfDescriptors.npos ? ListOfDescriptors.npos : end_of_module + 1;
        continue;
      }

      current_module->second.ModuleTSODisabled = false;

      // Walk all the address ranges provided.
      for (size_t non_tso_region_offset = 0; non_tso_region_offset != section_str.npos;) {
        size_t end_of_region_substr = section_str.find(",", non_tso_region_offset);
        std::string_view tso_region_view = section_str.substr(non_tso_region_offset, end_of_region_substr - non_tso_region_offset);

        if (tso_region_view.empty()) {
          non_tso_region_offset = end_of_region_substr == section_str.npos ? section_str.npos : end_of_region_substr + 1;
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
        non_tso_region_offset = end_of_region_substr == section_str.npos ? section_str.npos : end_of_region_substr + 1;
      }
    }

    // Individual instruction handling
    if (end_of_address_ranges != module_config.npos) {
      end_of_individual_inst = module_config.find(";", end_of_address_ranges + 1);
      std::string_view section_str = module_config.substr(end_of_address_ranges + 1, end_of_individual_inst);

      if (section_str.empty()) {
        module_offset = end_of_module == ListOfDescriptors.npos ? ListOfDescriptors.npos : end_of_module + 1;
        continue;
      }

      for (size_t force_tso_region_offset = 0; force_tso_region_offset != section_str.npos;) {
        size_t end_of_region_substr = section_str.find(",", force_tso_region_offset);
        std::string_view tso_region_view = section_str.substr(force_tso_region_offset, end_of_region_substr - force_tso_region_offset);

        if (tso_region_view.empty()) {
          force_tso_region_offset = end_of_region_substr == section_str.npos ? section_str.npos : end_of_region_substr + 1;
          continue;
        }

        uint64_t offset {};
        char* str_end;
        offset = std::strtoull(tso_region_view.data(), &str_end, 16);
        LOGMAN_THROW_A_FMT(tso_region_view.data() != str_end, "Couldn't parse offset {}", tso_region_view);

        current_module->second.VolatileInstructions.insert(offset);

        force_tso_region_offset = end_of_region_substr == section_str.npos ? section_str.npos : end_of_region_substr + 1;
      }
    }

    module_offset = end_of_module == ListOfDescriptors.npos ? ListOfDescriptors.npos : end_of_module + 1;
  }

  return ExtendedMetaData;
}
} // namespace FEX::VolatileMetadata
