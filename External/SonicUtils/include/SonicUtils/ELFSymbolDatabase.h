#pragma once
#include "ELFLoader.h"
#include <vector>
#include <unordered_map>

namespace ELFLoader {
class ELFSymbolDatabase final {
public:
  ELFSymbolDatabase(::ELFLoader::ELFContainer *file);
  ~ELFSymbolDatabase();

  ::ELFLoader::ELFContainer::MemoryLayout GetFileLayout() const;

  void MapMemoryRegions(std::function<void*(uint64_t, uint64_t, bool, bool)> Mapper);
  void WriteLoadableSections(::ELFLoader::ELFContainer::MemoryWriter Writer);

  uint64_t DefaultRIP() const;

  using RangeType = std::pair<uint64_t, uint64_t>;
  ::ELFLoader::ELFSymbol const *GetSymbolInRange(RangeType Address);
  ::ELFLoader::ELFSymbol const *GetGlobalSymbolInRange(RangeType Address);
  ::ELFLoader::ELFSymbol const *GetNoWeakSymbolInRange(RangeType Address);

  void GetInitLocations(std::vector<uint64_t> *Locations);
  uint64_t InitializeThreadSlot(std::function<void(void const*, uint64_t)> Writer) const;

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
  std::vector<ELFInfo*> DynamicELFInfo;
  std::vector<ELFInfo*> InitializationOrder;

  std::unordered_map<std::string, ELFInfo*> NameToELF;
  std::vector<std::string> LibrarySearchPaths;

  // Symbols
  std::vector<ELFLoader::ELFSymbol*> Symbols;
  using SymbolTableType = std::unordered_map<std::string, ELFLoader::ELFSymbol *>;
  SymbolTableType SymbolMap;
  SymbolTableType SymbolMapGlobalOnly;
  SymbolTableType SymbolMapNoWeak;
  SymbolTableType SymbolMapNoMain;
  SymbolTableType SymbolMapNoMainNoWeak;
  std::map<uint64_t, ELFLoader::ELFSymbol *> SymbolMapByAddress;

  bool FindLibraryFile(std::string *Result, const char *Library);
  void FillLibrarySearchPaths();
  void FillMemoryLayouts();
  void FillInitializationOrder();
  void FillSymbols();

  void HandleRelocations();

  ::ELFLoader::ELFSymbol const *GetSymbolFromTable(RangeType Address, std::unordered_map<std::string, ELFLoader::ELFSymbol *> &Table);
};
}

