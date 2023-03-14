#pragma once

#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/vector.h>

#include "Linux/Utils/ELFContainer.h"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <utility>

namespace ELFLoader {
class ELFSymbolDatabase final {
public:
  ELFSymbolDatabase(::ELFLoader::ELFContainer *file);
  ~ELFSymbolDatabase();

  uint64_t GetElfBase() const;

  void MapMemoryRegions(std::function<void*(uint64_t, uint64_t, bool)> Mapper);
  void WriteLoadableSections(::ELFLoader::ELFContainer::MemoryWriter Writer);

  uint64_t DefaultRIP() const;

  using RangeType = std::pair<uint64_t, uint64_t>;
  ::ELFLoader::ELFSymbol const *GetSymbolInRange(RangeType Address);
  ::ELFLoader::ELFSymbol const *GetGlobalSymbolInRange(RangeType Address);
  ::ELFLoader::ELFSymbol const *GetNoWeakSymbolInRange(RangeType Address);

  void GetInitLocations(fextl::vector<uint64_t> *Locations);

private:
  ::ELFLoader::ELFContainer *File;

  struct ELFInfo {
    std::string Name;
    ::ELFLoader::ELFContainer* Container;
    ::ELFLoader::ELFContainer::MemoryLayout CustomLayout;
    void *ELFBase;
    uint64_t GuestBase;
  };

  ELFInfo LocalInfo;
  fextl::vector<ELFInfo*> DynamicELFInfo;
  fextl::vector<ELFInfo*> InitializationOrder;
  uint64_t ELFMemorySize{};
  bool FixedNoReplace {true};
  void *ELFBase{};

  fextl::unordered_map<std::string, ELFInfo*> NameToELF;
  fextl::vector<std::string> LibrarySearchPaths;

  // Symbols
  fextl::vector<ELFLoader::ELFSymbol*> Symbols;
  using SymbolTableType = fextl::unordered_map<std::string, ELFLoader::ELFSymbol *>;
  SymbolTableType SymbolMap;
  SymbolTableType SymbolMapGlobalOnly;
  SymbolTableType SymbolMapNoWeak;
  SymbolTableType SymbolMapNoMain;
  SymbolTableType SymbolMapNoMainNoWeak;
  std::map<uint64_t, ELFLoader::ELFSymbol *> SymbolMapByAddress;

  bool FindLibraryFile(std::string *Result, const char *Library);
  void FillLibrarySearchPaths();
  void FillMemoryLayouts(uint64_t DefinedBase);
  void FillInitializationOrder();
  void FillSymbols();

  void HandleRelocations();

  ::ELFLoader::ELFSymbol const *GetSymbolFromTable(RangeType Address, fextl::unordered_map<std::string, ELFLoader::ELFSymbol *> &Table);
};
}

