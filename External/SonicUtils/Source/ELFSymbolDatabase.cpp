#include <SonicUtils/ELFSymbolDatabase.h>
#include <SonicUtils/LogManager.h>
#include <SonicUtils/Common/MathUtils.h>

#include <cstring>
#include <elf.h>
#include <filesystem>
#include <set>
#include <string>
#include <sys/stat.h>

namespace ELFLoader {
void ELFSymbolDatabase::FillLibrarySearchPaths() {
  // XXX: Open /etc/ld.so.conf and parse the paths in that file
  // For now I'm just filling with regular search paths
  if (File->GetMode() == ELFContainer::MODE_64BIT) {
    LibrarySearchPaths.emplace_back("/usr/local/lib/x86_64-linux-gnu");
    LibrarySearchPaths.emplace_back("/lib/x86_64-linux-gnu");
    LibrarySearchPaths.emplace_back("/usr/lib/x86_64-linux-gnu");
  }
  else {
    LibrarySearchPaths.emplace_back("/usr/local/lib/i386-linux-gnu");
    LibrarySearchPaths.emplace_back("/lib/i386-linux-gnu");
    LibrarySearchPaths.emplace_back("/usr/lib/i386-linux-gnu");
  }

  // At least we can scan LD_LIBRARY_PATH
  auto EnvVar = getenv("LD_LIBRARY_PATH");
  if (EnvVar) {
    std::string Env = EnvVar;
    std::stringstream EnvStream(Env);
    std::string Token;
    while (std::getline(EnvStream, Token, ';')) {
      LibrarySearchPaths.emplace_back(Token);
    }
  }
}

bool ELFSymbolDatabase::FindLibraryFile(std::string *Result, const char *Library) {
  for (auto &Path : LibrarySearchPaths) {
    std::string TmpPath = Path + "/" + Library;
    struct stat buf;
    // XXX: std::filesystem::exists was crashing in std::filesystem::path's destructor?
    if (stat(TmpPath.c_str(), &buf) == 0) {
      *Result = TmpPath;
      return true;
    }
  }
  return false;
}

ELFSymbolDatabase::ELFSymbolDatabase(::ELFLoader::ELFContainer *file)
  : File {file} {
  FillLibrarySearchPaths();

  std::vector<std::string> UnfilledDependencies;
  std::vector<ELFInfo*> NewLibraries;

  auto FillDependencies = [&UnfilledDependencies, this](ELFInfo *ELF) {
    for (auto &Lib : *ELF->Container->GetNecessaryLibs()) {
      if (NameToELF.find(Lib) == NameToELF.end()) {
        UnfilledDependencies.emplace_back(Lib);
      }
    }
  };

  auto LoadDependencies = [&UnfilledDependencies, &NewLibraries, this]() {
    for (auto &Lib : UnfilledDependencies) {
      if (NameToELF.find(Lib) == NameToELF.end()) {
        std::string LibraryPath;
        bool Found = FindLibraryFile(&LibraryPath, Lib.c_str());
        LogMan::Throw::A(Found, "Couldn't find library '%s'", Lib.c_str());
        auto Info = DynamicELFInfo.emplace_back(new ELFInfo{});
        Info->Name = Lib;
        Info->Container = new ::ELFLoader::ELFContainer(LibraryPath, {}, true);
        NewLibraries.emplace_back(Info);
        NameToELF[Lib] = Info;
      }
    }
  };

  LocalInfo.Container = File;
  LocalInfo.Name = "/proc/self/exe";
  NewLibraries.emplace_back(&LocalInfo);

  do {
    std::vector<ELFInfo*> PreviousLibs;
    PreviousLibs.swap(NewLibraries);
    for (auto ELF : PreviousLibs) {
      FillDependencies(ELF);
      LoadDependencies();
    }
  } while (!UnfilledDependencies.empty() && !NewLibraries.empty());

  FillMemoryLayouts();
  FillInitializationOrder();
  FillSymbols();
}

ELFSymbolDatabase::~ELFSymbolDatabase() {
}

void ELFSymbolDatabase::FillMemoryLayouts() {
  uint64_t ELFBases = 0x1'0000'0000;
  // We can only relocate the passed in ELF if it is dynamic
  // If it is EXEC then it HAS to end up in the base offset it chose
  if (LocalInfo.Container->WasDynamic()) {
    LocalInfo.CustomLayout = File->GetLayout();
    uint64_t CurrentELFBase = std::get<0>(LocalInfo.CustomLayout);
    uint64_t CurrentELFEnd = std::get<1>(LocalInfo.CustomLayout);
    uint64_t CurrentELFAlignedSize = AlignUp(std::get<2>(LocalInfo.CustomLayout), 4096);

    CurrentELFBase += ELFBases;
    CurrentELFEnd += ELFBases;

    std::get<0>(LocalInfo.CustomLayout) = CurrentELFBase;
    std::get<1>(LocalInfo.CustomLayout) = CurrentELFEnd;
    std::get<2>(LocalInfo.CustomLayout) = CurrentELFAlignedSize;
    LocalInfo.GuestBase = ELFBases;

    ELFBases += AlignUp(CurrentELFAlignedSize, 0x1'0000'0000);
  }
  else {
    LocalInfo.CustomLayout = File->GetLayout();
    uint64_t CurrentELFBase = std::get<0>(LocalInfo.CustomLayout);
    uint64_t CurrentELFEnd = std::get<1>(LocalInfo.CustomLayout);
    uint64_t CurrentELFAlignedSize = AlignUp(std::get<2>(LocalInfo.CustomLayout), 4096);

    std::get<0>(LocalInfo.CustomLayout) = CurrentELFBase;
    std::get<1>(LocalInfo.CustomLayout) = CurrentELFEnd;
    std::get<2>(LocalInfo.CustomLayout) = CurrentELFAlignedSize;
    LocalInfo.GuestBase = CurrentELFBase;
  }

  for (size_t i = 0; i < DynamicELFInfo.size(); ++i) {
    auto ELF = DynamicELFInfo[i]->Container;
    auto Layout = ELF->GetLayout();

    uint64_t CurrentELFBase = std::get<0>(Layout);
    uint64_t CurrentELFEnd = std::get<1>(Layout);
    uint64_t CurrentELFAlignedSize = AlignUp(std::get<2>(Layout), 4096);

    CurrentELFBase += ELFBases;
    CurrentELFEnd += ELFBases;

    std::get<0>(Layout) = CurrentELFBase;
    std::get<1>(Layout) = CurrentELFEnd;
    std::get<2>(Layout) = CurrentELFAlignedSize;
    DynamicELFInfo[i]->CustomLayout = Layout;
    DynamicELFInfo[i]->GuestBase = ELFBases;

    ELFBases += AlignUp(CurrentELFAlignedSize, 0x1'0000'0000);
  }
}

void ELFSymbolDatabase::FillInitializationOrder() {
  std::set<std::string> AlreadyInList;

  while (true) {
    if (InitializationOrder.size() == DynamicELFInfo.size())
      break;

    for (auto &ELF : DynamicELFInfo) {
      // If this ELF is already in the list then skip it
      if (AlreadyInList.find(ELF->Name) != AlreadyInList.end())
        continue;

      bool AllLibsLoaded = true;
      for (auto &Lib : *ELF->Container->GetNecessaryLibs()) {
        if (AlreadyInList.find(Lib) == AlreadyInList.end()) {
          AllLibsLoaded = false;
          break;
        }
      }

      if (AllLibsLoaded) {
        InitializationOrder.emplace_back(ELF);
        AlreadyInList.insert(ELF->Name);
      }
    }
  }
}

void ELFSymbolDatabase::FillSymbols() {
  auto LocalSymbolFiller = [this](ELFLoader::ELFSymbol *Symbol) {
    Symbols.emplace_back(Symbol);
    Symbol->Address += LocalInfo.GuestBase;
    SymbolMap[Symbol->Name] = Symbol;
    SymbolMapByAddress[Symbol->Address] = Symbol;
    if (Symbol->Bind == STB_GLOBAL) {
      SymbolMapGlobalOnly[Symbol->Name] = Symbol;
    }
    if (Symbol->Bind != STB_WEAK) {
      SymbolMapNoWeak[Symbol->Name] = Symbol;
    }
  };

  LocalInfo.Container->AddSymbols(LocalSymbolFiller);

  // Let us fill symbols based on initialization order
  for (auto ELF : InitializationOrder) {
    auto SymbolFiller = [this, &ELF](ELFLoader::ELFSymbol *Symbol) {
      Symbols.emplace_back(Symbol);
      // Offset the address by the guest base
      Symbol->Address += ELF->GuestBase;
      SymbolMap[Symbol->Name] = Symbol;
      SymbolMapNoMain[Symbol->Name] = Symbol;
      SymbolMapByAddress[Symbol->Address] = Symbol;
      if (Symbol->Bind == STB_GLOBAL) {
        SymbolMapGlobalOnly[Symbol->Name] = Symbol;
      }
      if (Symbol->Bind != STB_WEAK) {
        SymbolMapNoWeak[Symbol->Name] = Symbol;
        SymbolMapNoMainNoWeak[Symbol->Name] = Symbol;
      }
    };

    ELF->Container->AddSymbols(SymbolFiller);
  }

}

void ELFSymbolDatabase::MapMemoryRegions(std::function<void*(uint64_t, uint64_t, bool, bool)> Mapper) {
  // Need some special handling for the first 0x1'0000
  uint64_t ELFBase = std::get<0>(LocalInfo.CustomLayout);
  uint64_t ELFSize = std::get<2>(LocalInfo.CustomLayout);
  if (ELFBase == 0 && !LocalInfo.Container->WasDynamic()) {
    // Some special handling of non-dynamic applications that are mapped at 0
    ELFBase = 0x1'0000;
    ELFSize -= 0x1'0000;
    LocalInfo.ELFBase = Mapper(ELFBase, ELFSize, true, false);
    LocalInfo.ELFBase = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(LocalInfo.ELFBase) - 0x1'0000);
  }
  else {
    LocalInfo.ELFBase = Mapper(ELFBase, ELFSize, true, false);
  }

  for (auto &ELF : DynamicELFInfo) {
    ELF->ELFBase = Mapper(std::get<0>(ELF->CustomLayout), std::get<2>(ELF->CustomLayout), true, false);
  }
}

void ELFSymbolDatabase::WriteLoadableSections(::ELFLoader::ELFContainer::MemoryWriter Writer) {
  File->WriteLoadableSections(Writer, std::get<0>(LocalInfo.CustomLayout) - std::get<0>(File->GetLayout()));
  for (size_t i = 0; i < DynamicELFInfo.size(); ++i) {
    auto ELF = DynamicELFInfo[i]->Container;
    auto &Layout = DynamicELFInfo[i]->CustomLayout;

    ELF->WriteLoadableSections(Writer, std::get<0>(Layout) - std::get<0>(ELF->GetLayout()));
  }

  HandleRelocations();
}

void ELFSymbolDatabase::HandleRelocations() {
  auto SymbolGetter = [this](char const *SymbolName, uint8_t Table) -> ELFLoader::ELFSymbol* {
    SymbolTableType &TablePtr = SymbolMap;
    if (Table == 0)
      TablePtr = SymbolMap;
    else if (Table == 1) // Global
      TablePtr = SymbolMapGlobalOnly;
    else if (Table == 2) // NoWeak
      TablePtr = SymbolMapNoWeak;
    else if (Table == 3) // No Main
      TablePtr = SymbolMapNoMain;
    else if (Table == 4) // No Main No Weak
      TablePtr = SymbolMapNoMainNoWeak;

    auto Sym = TablePtr.find(SymbolName);
    if (Sym == TablePtr.end())
      return nullptr;
    return Sym->second;
  };

  for (auto ELF : InitializationOrder) {
    ELF->Container->FixupRelocations(ELF->ELFBase, ELF->GuestBase, SymbolGetter);
  }

  LocalInfo.Container->FixupRelocations(LocalInfo.ELFBase, LocalInfo.GuestBase, SymbolGetter);
}

::ELFLoader::ELFContainer::MemoryLayout ELFSymbolDatabase::GetFileLayout() const {
  return LocalInfo.CustomLayout;
}

uint64_t ELFSymbolDatabase::DefaultRIP() const {
  return File->GetEntryPoint() + std::get<0>(LocalInfo.CustomLayout);
}

ELFSymbol const *ELFSymbolDatabase::GetSymbolInRange(RangeType Address) {
  auto Sym = SymbolMapByAddress.upper_bound(Address.first);
  if (Sym != SymbolMapByAddress.begin())
    --Sym;
  if (Sym == SymbolMapByAddress.end())
    return nullptr;

  if ((Sym->second->Address + Sym->second->Size) < Address.first)
    return nullptr;

  if (Sym->second->Address > Address.first)
    return nullptr;

  return Sym->second;
}

void ELFSymbolDatabase::GetInitLocations(std::vector<uint64_t> *Locations) {
  // Walk the initialization order and fill the locations for initializations
  for (auto ELF : InitializationOrder) {
    ELF->Container->GetInitLocations(ELF->GuestBase, Locations);
  }
}

uint64_t ELFSymbolDatabase::InitializeThreadSlot(std::function<void(void const*, uint64_t)> Writer) const {
  for (auto ELF : InitializationOrder) {
    if (ELF->Container->HasTLS()) {
      return ELF->Container->InitializeThreadSlot(ELF->ELFBase, Writer);
    }
  }

  return 0;
}

}
