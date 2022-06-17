#pragma once
#include <algorithm>
#include <optional>
#include <string>
#include <vector>
#include <memory>

namespace FEXCore::IR {
  struct AOTIRCacheEntry;
}

namespace FEXCore::HLE {

struct SourcecodeLineMapping {
  uintptr_t FileGuestBegin;
  uintptr_t FileGuestEnd;
  
  int LineNumber;
};

struct SourcecodeSymbolMapping {
  uintptr_t FileGuestBegin;
  uintptr_t FileGuestEnd;

  std::string Name;
};

struct SourcecodeMap {
  std::string SourceFile;
  std::vector<SourcecodeLineMapping> SortedLineMappings;
  std::vector<SourcecodeSymbolMapping> SortedSymbolMappings;

  template<typename F>
  void IterateLineMappings(uintptr_t FileBegin, uintptr_t Size, const F &Callback) const {
    auto Begin = FileBegin;
    auto End = FileBegin + Size;

    auto Found = std::lower_bound(SortedLineMappings.cbegin(), SortedLineMappings.cend(), Begin, [](const auto &Range, const auto Position) {
      return Range.FileGuestEnd <= Position;
    });

    while (Found != SortedLineMappings.cend()) {
      if (Found->FileGuestBegin < End && Found->FileGuestEnd > Begin) {
        Callback(Found);
      } else {
        break;
      }
      Found++;
    }
  }

  const SourcecodeLineMapping *FindLineMapping(uintptr_t FileBegin) const {
    return Find(FileBegin, SortedLineMappings);
  }

  const SourcecodeSymbolMapping *FindSymbolMapping(uintptr_t FileBegin) const {
    return Find(FileBegin, SortedSymbolMappings);
  }
private:
  template<typename VecT>
  const typename VecT::value_type *Find(uintptr_t FileBegin, const VecT &SortedMappings) const {
    auto Found = std::lower_bound(SortedMappings.cbegin(), SortedMappings.cend(), FileBegin, [](const auto &Range, const auto Position) {
      return Range.FileGuestEnd <= Position;
    });

    if (Found != SortedMappings.end() && Found->FileGuestBegin <= FileBegin && Found->FileGuestEnd > FileBegin) {
      return &(*Found);
    } else {
      return {};
    }
  }
};

class SourcecodeResolver {
public:
  virtual std::unique_ptr<SourcecodeMap> GenerateMap(const std::string_view& GuestBinaryFile, const std::string_view& GuestBinaryFileId) = 0;
};
}
