// SPDX-License-Identifier: MIT
/*
$info$
tags: glue|elf-parsing
desc: Part of our now defunct ld-linux replacement, keeps tracks of all symbols, loads elfs, handles relocations. Small parts of this are used.
$end_info$
*/

#include "Linux/Utils/ELFSymbolDatabase.h"

#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/Filesystem.h>

#include <cstdlib>
#include <cstring>
#include <elf.h>
#include <set>
#include <sys/mman.h>
#include <sys/stat.h>
#include <tuple>

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
    fextl::string Env = EnvVar;
    fextl::stringstream EnvStream(Env);
    fextl::string Token;
    while (std::getline(EnvStream, Token, ';')) {
      LibrarySearchPaths.emplace_back(Token);
    }
  }
}

bool ELFSymbolDatabase::FindLibraryFile(fextl::string *Result, const char *Library) {
  for (auto &Path : LibrarySearchPaths) {
    const fextl::string TmpPath = fextl::fmt::format("{}/{}", Path, Library);
    if (FHU::Filesystem::Exists(TmpPath)) {
      *Result = TmpPath;
      return true;
    }
  }
  return false;
}

ELFSymbolDatabase::ELFSymbolDatabase(::ELFLoader::ELFContainer *file)
  : File {file} {
  FillLibrarySearchPaths();

  // Don't try to load the file if it was invalid
  if (!File->WasLoaded()) {
    return;
  }

  fextl::vector<fextl::string> UnfilledDependencies;
  fextl::vector<ELFInfo*> NewLibraries;

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
        fextl::string LibraryPath;
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
        bool Found =
#endif
        FindLibraryFile(&LibraryPath, Lib.c_str());
        LOGMAN_THROW_A_FMT(Found, "Couldn't find library '{}'", Lib);
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
    fextl::vector<ELFInfo*> PreviousLibs;
    PreviousLibs.swap(NewLibraries);
    for (auto ELF : PreviousLibs) {
      FillDependencies(ELF);
      LoadDependencies();
    }
  } while (!UnfilledDependencies.empty() && !NewLibraries.empty());

  FillMemoryLayouts(0);
  FillInitializationOrder();
  FillSymbols();

  if (LocalInfo.Container->WasDynamic() && File->GetMode() == ELFContainer::MODE_64BIT) {
    ELFBase = FEXCore::Allocator::mmap(nullptr, ELFMemorySize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    FillMemoryLayouts(reinterpret_cast<uintptr_t>(ELFBase));
    FillInitializationOrder();
    FillSymbols();

    FixedNoReplace = false;
  }
}

ELFSymbolDatabase::~ELFSymbolDatabase() {
  if (ELFBase) {
    FEXCore::Allocator::munmap(ELFBase, ELFMemorySize);
    ELFBase = nullptr;
  }
}

void ELFSymbolDatabase::FillMemoryLayouts(uint64_t DefinedBase) {
  uint64_t ELFBases = DefinedBase;
  ELFMemorySize = 0;
  if (!DefinedBase) {
    if (File->GetMode() == ELFContainer::MODE_64BIT) {
      ELFBases = 0x1'0000'0000;
    }
    else {
      // 32bit we will just load at the lowest memory address we can
      // Which on Linux is at 0x1'0000
      ELFBases = 0x1'0000;
    }
  }
  // We can only relocate the passed in ELF if it is dynamic
  // If it is EXEC then it HAS to end up in the base offset it chose
  if (LocalInfo.Container->WasDynamic()) {
    LocalInfo.CustomLayout = File->GetLayout();
    uint64_t CurrentELFBase = std::get<0>(LocalInfo.CustomLayout);
    uint64_t CurrentELFEnd = std::get<1>(LocalInfo.CustomLayout);
    uint64_t CurrentELFAlignedSize = FEXCore::AlignUp(std::get<2>(LocalInfo.CustomLayout), 4096);

    CurrentELFBase += ELFBases;
    CurrentELFEnd += ELFBases;

    std::get<0>(LocalInfo.CustomLayout) = CurrentELFBase;
    std::get<1>(LocalInfo.CustomLayout) = CurrentELFEnd;
    std::get<2>(LocalInfo.CustomLayout) = CurrentELFAlignedSize;
    LocalInfo.GuestBase = ELFBases;

    ELFBases += CurrentELFAlignedSize;
    ELFMemorySize += CurrentELFAlignedSize;
  }
  else {
    LocalInfo.CustomLayout = File->GetLayout();
    uint64_t CurrentELFBase = std::get<0>(LocalInfo.CustomLayout);
    uint64_t CurrentELFAlignedSize = FEXCore::AlignUp(std::get<2>(LocalInfo.CustomLayout), 4096);
    if (CurrentELFBase < 0x10000) {
      // We can't allocate memory in the first 16KB,  Hopefully no elfs require this.
      LOGMAN_MSG_A_FMT("Elf requires memory mapped in the first 16kb");
    }

    std::get<2>(LocalInfo.CustomLayout) = CurrentELFAlignedSize;
    LocalInfo.GuestBase = 0;
    ELFMemorySize += CurrentELFAlignedSize;
  }

  for (size_t i = 0; i < DynamicELFInfo.size(); ++i) {
    auto ELF = DynamicELFInfo[i]->Container;
    auto Layout = ELF->GetLayout();

    uint64_t CurrentELFBase = std::get<0>(Layout);
    uint64_t CurrentELFEnd = std::get<1>(Layout);
    uint64_t CurrentELFAlignedSize = FEXCore::AlignUp(std::get<2>(Layout), 4096);

    CurrentELFBase += ELFBases;
    CurrentELFEnd += ELFBases;

    std::get<0>(Layout) = CurrentELFBase;
    std::get<1>(Layout) = CurrentELFEnd;
    std::get<2>(Layout) = CurrentELFAlignedSize;
    DynamicELFInfo[i]->CustomLayout = Layout;
    DynamicELFInfo[i]->GuestBase = ELFBases;

    ELFBases += CurrentELFAlignedSize;
    ELFMemorySize += CurrentELFAlignedSize;
  }
}

void ELFSymbolDatabase::FillInitializationOrder() {
  std::set<fextl::string> AlreadyInList;

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

void ELFSymbolDatabase::MapMemoryRegions(std::function<void*(uint64_t, uint64_t, bool)> Mapper) {
  auto Map = [&](ELFInfo& ELF) {
    uint64_t ELFBase = std::get<0>(ELF.CustomLayout);
    uint64_t ELFSize = std::get<2>(ELF.CustomLayout);
    uint64_t OffsetFromBase = ELFBase - ELF.GuestBase;
    ELF.ELFBase = static_cast<uint8_t*>(Mapper(ELFBase, ELFSize, FixedNoReplace)) - OffsetFromBase;
  };

  Map(LocalInfo);
  for (auto *ELF : DynamicELFInfo) {
    Map(*ELF);
  }
}

void ELFSymbolDatabase::WriteLoadableSections(::ELFLoader::ELFContainer::MemoryWriter Writer) {
  File->WriteLoadableSections(Writer, LocalInfo.GuestBase);

  for (size_t i = 0; i < DynamicELFInfo.size(); ++i) {
    auto ELF = DynamicELFInfo[i]->Container;

    ELF->WriteLoadableSections(Writer, DynamicELFInfo[i]->GuestBase);
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

uint64_t ELFSymbolDatabase::GetElfBase() const {
  return LocalInfo.GuestBase;
}

uint64_t ELFSymbolDatabase::DefaultRIP() const {
  return File->GetEntryPoint() + LocalInfo.GuestBase;
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

void ELFSymbolDatabase::GetInitLocations(fextl::vector<uint64_t> *Locations) {
  // Walk the initialization order and fill the locations for initializations
  for (auto ELF : InitializationOrder) {
    ELF->Container->GetInitLocations(ELF->GuestBase, Locations);
  }
}

}
