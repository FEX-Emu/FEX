// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <algorithm>
#include <memory>
#include <filesystem>

namespace FEXCore::HLE {

struct SourcecodeLineMapping {
  uintptr_t FileGuestBegin;
  uintptr_t FileGuestEnd;

  int LineNumber;
};

struct SourcecodeSymbolMapping {
  uintptr_t FileGuestBegin;
  uintptr_t FileGuestEnd;

  fextl::string Name;

  static fextl::string SymName(const SourcecodeSymbolMapping* Sym, const fextl::string& GuestFilename, uintptr_t HostEntry, uintptr_t FileBegin) {
    if (Sym) {
      auto SymOffset = FileBegin - Sym->FileGuestBegin;
      if (SymOffset) {
        return fextl::fmt::format("{}: {}+{} @{:x}", std::filesystem::path(GuestFilename).stem().string(), Sym->Name, SymOffset, HostEntry);
      } else {
        return fextl::fmt::format("{}: {} @{:x}", std::filesystem::path(GuestFilename).stem().string(), Sym->Name, HostEntry);
      }
    } else {
      return fextl::fmt::format("{}: +{} @{:x}", std::filesystem::path(GuestFilename).stem().string(), FileBegin, HostEntry);
    }
  }
};

struct SourcecodeMap {
  fextl::string SourceFile;
  fextl::vector<SourcecodeLineMapping> SortedLineMappings;
  fextl::vector<SourcecodeSymbolMapping> SortedSymbolMappings;

  template<typename F>
  void IterateLineMappings(uintptr_t FileBegin, uintptr_t Size, const F& Callback) const {
    auto Begin = FileBegin;
    auto End = FileBegin + Size;

    auto Found = std::lower_bound(SortedLineMappings.cbegin(), SortedLineMappings.cend(), Begin,
                                  [](const auto& Range, const auto Position) { return Range.FileGuestEnd <= Position; });

    while (Found != SortedLineMappings.cend()) {
      if (Found->FileGuestBegin < End && Found->FileGuestEnd > Begin) {
        Callback(Found);
      } else {
        break;
      }
      Found++;
    }
  }

  const SourcecodeLineMapping* FindLineMapping(uintptr_t FileBegin) const {
    return Find(FileBegin, SortedLineMappings);
  }

  const SourcecodeSymbolMapping* FindSymbolMapping(uintptr_t FileBegin) const {
    return Find(FileBegin, SortedSymbolMappings);
  }
private:
  template<typename VecT>
  const typename VecT::value_type* Find(uintptr_t FileBegin, const VecT& SortedMappings) const {
    auto Found = std::lower_bound(SortedMappings.cbegin(), SortedMappings.cend(), FileBegin,
                                  [](const auto& Range, const auto Position) { return Range.FileGuestEnd <= Position; });

    if (Found != SortedMappings.end() && Found->FileGuestBegin <= FileBegin && Found->FileGuestEnd > FileBegin) {
      return &(*Found);
    } else {
      return {};
    }
  }
};

class SourcecodeResolver {
public:
  virtual fextl::unique_ptr<SourcecodeMap> GenerateMap(std::string_view GuestBinaryFile, std::string_view GuestBinaryFileId) = 0;
};
} // namespace FEXCore::HLE
