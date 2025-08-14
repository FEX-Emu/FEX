// SPDX-License-Identifier: MIT
#include "Common/VolatileMetadata.h"

#include <charconv>

namespace FEX::VolatileMetadata {
fextl::unordered_map<fextl::string, ExtendedVolatileMetadata> ParseExtendedVolatileMetadata(std::string_view ListOfDescriptors) {
  // Parsing: `<module>;<address begin>-<address-end>,<more addresses>;<instruction offset to force TSO>:`
  if (ListOfDescriptors.empty()) return {};

  fextl::unordered_map<fextl::string, ExtendedVolatileMetadata> ExtendedMetaData {};

  for (size_t module_offset = 0; module_offset != ListOfDescriptors.npos; ) {
    size_t end_of_module_offset = ListOfDescriptors.find(":", module_offset);
    std::string_view module_config = ListOfDescriptors.substr(module_offset, end_of_module_offset - module_offset);

    if (module_config.empty()) {
      module_offset = end_of_module_offset == ListOfDescriptors.npos ? ListOfDescriptors.npos : end_of_module_offset + 1;
      continue;
    }

    fextl::unordered_map<fextl::string, ExtendedVolatileMetadata>::iterator ModuleIter {};
    for (size_t module_substr_offset = 0, current_config = 0; module_substr_offset != module_config.npos; ++current_config) {
      // Walk the three sub_config options
      size_t end_of_module_substr = module_config.find(";", module_substr_offset);
      std::string_view module_sub_view = module_config.substr(module_substr_offset, end_of_module_substr - module_substr_offset);
      if (module_sub_view.empty()) {
        module_substr_offset = end_of_module_substr == module_config.npos ? module_config.npos : end_of_module_substr + 1;
        continue;
      }

      switch (current_config) {
        case 0: { // Module name
          ModuleIter = ExtendedMetaData.insert_or_assign(fextl::string(module_sub_view), ExtendedVolatileMetadata {
            .ModuleTSODisabled = true,
          }).first;
          break;
        }
        case 1: { // NonTSO region
          ModuleIter->second.ModuleTSODisabled = false;
          for (size_t non_tso_region_offset = 0; non_tso_region_offset != module_sub_view.npos; ) {
            size_t end_of_region_substr = module_sub_view.find(",", non_tso_region_offset);
            std::string_view tso_region_view = module_sub_view.substr(non_tso_region_offset, end_of_region_substr - non_tso_region_offset);

            if (tso_region_view.empty()) {
              non_tso_region_offset = end_of_region_substr == module_sub_view.npos ? module_sub_view.npos : end_of_region_substr + 1;
              continue;
            }

            uint64_t begin {}, end {};
            auto result = std::from_chars(tso_region_view.begin(), tso_region_view.end(), begin);
            if (result.ec != std::errc{}) {
              LOGMAN_MSG_A_FMT("Couldn't parse begin {}", tso_region_view);
            }

            if (result.ptr != tso_region_view.end()) {
              result = std::from_chars(result.ptr + 1, tso_region_view.end(), end);
              if (result.ec != std::errc{}) {
                LOGMAN_MSG_A_FMT("Couldn't parse end {}", tso_region_view);
              }
            }
            else {
              LOGMAN_MSG_A_FMT("Couldn't parse end {}", tso_region_view);
            }

            ModuleIter->second.VolatileValidRanges.Insert({begin, end});
            non_tso_region_offset = end_of_region_substr == module_sub_view.npos ? module_sub_view.npos : end_of_region_substr + 1;
          }
          break;
        }
        case 2: { // Volatile instructions region
          for (size_t force_tso_region_offset = 0; force_tso_region_offset != module_sub_view.npos; ) {
            size_t end_of_region_substr = module_sub_view.find(",", force_tso_region_offset);
            std::string_view tso_region_view = module_sub_view.substr(force_tso_region_offset, end_of_region_substr - force_tso_region_offset);

            if (tso_region_view.empty()) {
              force_tso_region_offset = end_of_region_substr == module_sub_view.npos ? module_sub_view.npos : end_of_region_substr + 1;
              continue;
            }

            uint64_t offset {};
            auto result = std::from_chars(tso_region_view.begin(), tso_region_view.end(), offset);
            if (result.ec != std::errc{}) {
              LOGMAN_MSG_A_FMT("Couldn't parse offset {}", tso_region_view);
            }

            ModuleIter->second.VolatileInstructions.insert(offset);

            force_tso_region_offset = end_of_region_substr == module_sub_view.npos ? module_sub_view.npos : end_of_region_substr + 1;
          }
          break;
        }
        default: LOGMAN_MSG_A_FMT("Unhandled extended volatile region: {}", current_config);
      }

      module_substr_offset = end_of_module_substr == module_config.npos ? module_config.npos : end_of_module_substr + 1;
    }

    module_offset = end_of_module_offset == ListOfDescriptors.npos ? ListOfDescriptors.npos : end_of_module_offset + 1;
  }

  return ExtendedMetaData;
}
}
