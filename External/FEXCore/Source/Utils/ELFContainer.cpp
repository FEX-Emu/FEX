/*
$info$
tags: glue|elf-parsing
desc: Loads and parses an elf to memory. Also handles some loading & logic.
$end_info$
*/

#include <FEXCore/Utils/Common/MathUtils.h>
#include <FEXCore/Utils/ELFContainer.h>
#include <FEXCore/Utils/LogManager.h>
#include <cstring>
#include <elf.h>
#include <filesystem>
#include <fstream>
#include <stdint.h>
#include <vector>

namespace ELFLoader {
  ELFContainer::ELFType ELFContainer::GetELFType(std::string const &Filename) {
  std::fstream ELFFile;
  size_t FileSize{0};
  ELFFile.open(Filename, std::fstream::in | std::fstream::binary);

  if (!ELFFile.is_open()) {
    return ELFType::TYPE_NONE;
  }

  ELFFile.seekg(0, ELFFile.end);
  FileSize = ELFFile.tellg();
  ELFFile.seekg(0, ELFFile.beg);

  size_t ELFHeaderSize = std::max(sizeof(Elf32_Ehdr), sizeof(Elf64_Ehdr));
  if (FileSize < ELFHeaderSize) {
    return ELFType::TYPE_NONE;
  }

  FileSize = ELFHeaderSize;

  std::vector<char> RawFile;
  RawFile.resize(FileSize);

  ELFFile.read(&RawFile.at(0), FileSize);

  ELFFile.close();

  uint8_t *Ident = reinterpret_cast<uint8_t*>(&RawFile.at(0));

  if (Ident[EI_MAG0] != ELFMAG0 ||
      Ident[EI_MAG1] != ELFMAG1 ||
      Ident[EI_MAG2] != ELFMAG2 ||
      Ident[EI_MAG3] != ELFMAG3) {
    return ELFType::TYPE_NONE;
  }

  union {
    Elf32_Ehdr _32;
    Elf64_Ehdr _64;
  } Header;

  if (Ident[EI_CLASS] == ELFCLASS32) {
    memcpy(&Header, reinterpret_cast<Elf32_Ehdr *>(&RawFile.at(0)),
      sizeof(Elf32_Ehdr));
    if (Header._32.e_machine == EM_386) {
      return ELFType::TYPE_X86_32;
    }
  }
  else if (Ident[EI_CLASS] == ELFCLASS64) {
    memcpy(&Header, reinterpret_cast<Elf64_Ehdr *>(&RawFile.at(0)),
      sizeof(Elf64_Ehdr));
    if (Header._64.e_machine == EM_X86_64) {
      return ELFType::TYPE_X86_64;
    }
  }

  return ELFType::TYPE_OTHER_ELF;
}

ELFContainer::ELFContainer(std::string const &Filename, std::string const &RootFS, bool CustomInterpreter) {
  Loaded = true;
  if (!LoadELF(Filename)) {
    LogMan::Msg::E("Couldn't Load ELF file");
    Loaded = false;
    return;
  }

  if (InterpreterHeader._64 && !CustomInterpreter) {
    // If we we are dynamic application then we have an interpreter program header
    // We need to load that ELF instead if it exists
    // We are no longer dynamic since we are executing the interpreter
    const char *RawString{};
    if (Mode == MODE_32BIT) {
      RawString = &RawFile.at(InterpreterHeader._32->p_offset);
    }
    else {
      RawString = &RawFile.at(InterpreterHeader._64->p_offset);
    }
    std::string RootFSLink = RootFS + RawString;
    while (std::filesystem::is_symlink(RootFSLink)) {
      // Do some special handling if the RootFS's linker is a symlink
      // Ubuntu's rootFS by default provides an absolute location symlink to the linker
      // Resolve this around back to the rootfs
      auto SymlinkTarget = std::filesystem::read_symlink(RootFSLink);
      if (SymlinkTarget.is_absolute()) {
        RootFSLink = RootFS + SymlinkTarget.string();
      }
      else {
        break;
      }
    }
    if (LoadELF(RootFSLink)) {
      // Found the interpreter in the rootfs
    }
    else if (!LoadELF(RawString)) {
      LogMan::Msg::E("Failed to find guest ELF's interpter '%s'", RawString);
      LogMan::Msg::E("Did you forget to set an x86 rootfs? Currently '%s'", RootFS.c_str());
      Loaded = false;
      return;
    }
  }
  else if (InterpreterHeader._64) {
    GetDynamicLibs();
  }


  CalculateMemoryLayouts();
  CalculateSymbols();

  // Print Information
  // PrintHeader();
  //PrintSectionHeaders();
  //PrintProgramHeaders();
  //PrintSymbolTable();
  //PrintRelocationTable();
  //PrintInitArray();
  //PrintDynamicTable();

  //LOGMAN_THROW_A(InterpreterHeader == nullptr, "Can only handle static programs");
}

ELFContainer::~ELFContainer() {
  NecessaryLibs.clear();
  SymbolMapByAddress.clear();
  SymbolMap.clear();
  Symbols.clear();
  ProgramHeaders.clear();
  SectionHeaders.clear();
  RawFile.clear();
}

bool ELFContainer::LoadELF(std::string const &Filename) {
  std::fstream ELFFile;
  size_t FileSize{0};
  ELFFile.open(Filename, std::fstream::in | std::fstream::binary);

  if (!ELFFile.is_open())
    return false;

  ELFFile.seekg(0, ELFFile.end);
  FileSize = ELFFile.tellg();
  ELFFile.seekg(0, ELFFile.beg);

  RawFile.resize(FileSize);

  ELFFile.read(&RawFile.at(0), FileSize);

  ELFFile.close();

  InterpreterHeader._64 = nullptr;

  SectionHeaders.clear();
  ProgramHeaders.clear();

  uint8_t *Ident = reinterpret_cast<uint8_t*>(&RawFile.at(0));

  if (Ident[EI_MAG0] != ELFMAG0 ||
      Ident[EI_MAG1] != ELFMAG1 ||
      Ident[EI_MAG2] != ELFMAG2 ||
      Ident[EI_MAG3] != ELFMAG3) {
    LogMan::Msg::E("ELF missing magic cookie");
    return false;
  }

  if (Ident[EI_CLASS] == ELFCLASS32) {
    return LoadELF_32();
  }
  else if (Ident[EI_CLASS] == ELFCLASS64) {
    return LoadELF_64();
  }

  LogMan::Msg::E("Unknown ELF type");
  return false;
}

bool ELFContainer::LoadELF_32() {
  Mode = MODE_32BIT;

  memcpy(&Header, reinterpret_cast<Elf32_Ehdr *>(&RawFile.at(0)),
         sizeof(Elf32_Ehdr));
  LOGMAN_THROW_A(Header._32.e_phentsize == sizeof(Elf32_Phdr), "PH Entry size wasn't correct size");
  LOGMAN_THROW_A(Header._32.e_shentsize == sizeof(Elf32_Shdr), "PH Entry size wasn't correct size");

  if (Header._32.e_machine != EM_386) {
    LogMan::Msg::D("32bit ELF wasn't x86 based");
    return false;
  }

  SectionHeaders.resize(Header._32.e_shnum);
  ProgramHeaders.resize(Header._32.e_phnum);

  Elf32_Shdr *RawShdrs =
      reinterpret_cast<Elf32_Shdr *>(&RawFile.at(Header._32.e_shoff));
  Elf32_Phdr *RawPhdrs =
      reinterpret_cast<Elf32_Phdr *>(&RawFile.at(Header._32.e_phoff));

  for (uint32_t i = 0; i < Header._32.e_shnum; ++i) {
    SectionHeaders[i]._32 = &RawShdrs[i];
  }

  for (uint32_t i = 0; i < Header._32.e_phnum; ++i) {
    ProgramHeaders[i]._32 = &RawPhdrs[i];
    if (ProgramHeaders[i]._32->p_type == PT_INTERP) {
      InterpreterHeader = ProgramHeaders[i];
      DynamicLinker = reinterpret_cast<char const*>(&RawFile.at(InterpreterHeader._32->p_offset));
    }
  }

  DynamicProgram = Header._32.e_type != ET_EXEC;

  // Default BRK size
  BRKSize = 4096;

  return true;
}

bool ELFContainer::LoadELF_64() {
  Mode = MODE_64BIT;

  memcpy(&Header, reinterpret_cast<Elf64_Ehdr *>(&RawFile.at(0)),
         sizeof(Elf64_Ehdr));
  LOGMAN_THROW_A(Header._64.e_phentsize == 56, "PH Entry size wasn't 56");
  LOGMAN_THROW_A(Header._64.e_shentsize == 64, "PH Entry size wasn't 64");

  if (Header._64.e_machine != EM_X86_64) {
    LogMan::Msg::D("64bit ELF wasn't x86-64 based");
    return false;
  }

  SectionHeaders.resize(Header._64.e_shnum);
  ProgramHeaders.resize(Header._64.e_phnum);

  Elf64_Shdr *RawShdrs =
      reinterpret_cast<Elf64_Shdr *>(&RawFile.at(Header._64.e_shoff));
  Elf64_Phdr *RawPhdrs =
      reinterpret_cast<Elf64_Phdr *>(&RawFile.at(Header._64.e_phoff));

  for (uint32_t i = 0; i < Header._64.e_shnum; ++i) {
    SectionHeaders[i]._64 = &RawShdrs[i];
  }

  for (uint32_t i = 0; i < Header._64.e_phnum; ++i) {
    ProgramHeaders[i]._64 = &RawPhdrs[i];
    if (ProgramHeaders[i]._64->p_type == PT_INTERP) {
      InterpreterHeader = ProgramHeaders[i];
      DynamicLinker = reinterpret_cast<char const*>(&RawFile.at(InterpreterHeader._64->p_offset));
    }
  }

  DynamicProgram = Header._64.e_type != ET_EXEC;

  // Default BRK size
  BRKSize = 0x1000'0000;

  return true;
}

void ELFContainer::WriteLoadableSections(MemoryWriter Writer, uint64_t Offset) {
  if (Mode == MODE_32BIT) {
    for (uint32_t i = 0; i < ProgramHeaders.size(); ++i) {
      Elf32_Phdr const *hdr = ProgramHeaders.at(i)._32;
      if (hdr->p_type == PT_LOAD) {
        //LogMan::Msg::D("PT_LOAD: Base: %p Offset: [0x%x, 0x%x)", Offset, hdr->p_paddr, hdr->p_filesz);
        Writer(&RawFile.at(hdr->p_offset), Offset + hdr->p_paddr, hdr->p_filesz);
      }

      if (hdr->p_type == PT_TLS) {
        Writer(&RawFile.at(hdr->p_offset), Offset + hdr->p_paddr, hdr->p_filesz);
      }
    }
  }
  else {
    for (uint32_t i = 0; i < ProgramHeaders.size(); ++i) {
      Elf64_Phdr const *hdr = ProgramHeaders.at(i)._64;
      if (hdr->p_type == PT_LOAD) {
        Writer(&RawFile.at(hdr->p_offset), Offset + hdr->p_paddr, hdr->p_filesz);
      }

      if (hdr->p_type == PT_TLS) {
        Writer(&RawFile.at(hdr->p_offset), Offset + hdr->p_paddr, hdr->p_filesz);
      }
    }
  }
}

ELFSymbol const *ELFContainer::GetSymbol(char const *Name) {
  auto Sym = SymbolMap.find(Name);
  if (Sym == SymbolMap.end())
    return nullptr;
  return Sym->second;
}
ELFSymbol const *ELFContainer::GetSymbol(uint64_t Address) {
  auto Sym = SymbolMapByAddress.find(Address);
  if (Sym == SymbolMapByAddress.end())
    return nullptr;
  return Sym->second;
}
ELFSymbol const *ELFContainer::GetSymbolInRange(RangeType Address) {
  auto Sym = SymbolMapByAddress.upper_bound(Address.first);
  if (Sym != SymbolMapByAddress.begin())
    --Sym;
  if (Sym == SymbolMapByAddress.end())
    return nullptr;

  if ((Sym->second->Address + Sym->second->Size) < Address.first)
    return nullptr;

  return Sym->second;
}

void ELFContainer::CalculateMemoryLayouts() {
  uint64_t MinPhysAddr = ~0ULL;
  uint64_t MaxPhysAddr = 0;
  uint64_t PhysMemSize = 0;

  if (Mode == MODE_32BIT) {
    for (uint32_t i = 0; i < ProgramHeaders.size(); ++i) {
      Elf32_Phdr *hdr = ProgramHeaders.at(i)._32;
      if (hdr->p_memsz > 0) {
        MinPhysAddr = std::min(MinPhysAddr, static_cast<uint64_t>(hdr->p_paddr));
        MaxPhysAddr = std::max(MaxPhysAddr, static_cast<uint64_t>(hdr->p_paddr) + hdr->p_memsz);
      }
      if (hdr->p_type == PT_TLS) {
        TLSHeader._32 = hdr;
      }
    }
  }
  else {
    for (uint32_t i = 0; i < ProgramHeaders.size(); ++i) {
      Elf64_Phdr *hdr = ProgramHeaders.at(i)._64;

      // Many elfs have program region labeled .GNU_STACK which is empty and has a null address.
      // It's used to mark the memory protection flags of the stack.
      //
      // We need to ignore such empty sections, or we will mistakenly assume the elf starts at zero.
      if (hdr->p_memsz > 0) {
        MinPhysAddr = std::min(MinPhysAddr, static_cast<uint64_t>(hdr->p_paddr));
        MaxPhysAddr = std::max(MaxPhysAddr, static_cast<uint64_t>(hdr->p_paddr + hdr->p_memsz));
      }
      if (hdr->p_type == PT_TLS) {
        TLSHeader._64 = hdr;
      }
    }
  }

  // Calculate BRK
  MaxPhysAddr = AlignUp(MaxPhysAddr, 4096);
  BRKBase = MaxPhysAddr;
  MaxPhysAddr += BRKSize;

  PhysMemSize = MaxPhysAddr - MinPhysAddr;

  MinPhysicalMemoryLocation = MinPhysAddr;
  MaxPhysicalMemoryLocation = MaxPhysAddr;
  PhysicalMemorySize = PhysMemSize;
}

void ELFContainer::CalculateSymbols() {
  // Find the symbol table
  if (Mode == MODE_32BIT) {
    Elf32_Shdr const *SymTabHeader{nullptr};
    Elf32_Shdr const *StringTableHeader{nullptr};
    char const *StrTab{nullptr};

    Elf32_Shdr const *DynSymTabHeader{nullptr};
    Elf32_Shdr const *DynStringTableHeader{nullptr};
    char const *DynStrTab{nullptr};

    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf32_Shdr const *hdr = SectionHeaders.at(i)._32;
      if (hdr->sh_type == SHT_SYMTAB) {
        SymTabHeader = hdr;
        break;
      }
    }

    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf32_Shdr const *hdr = SectionHeaders.at(i)._32;
      if (hdr->sh_type == SHT_DYNSYM) {
        DynSymTabHeader = hdr;
        break;
      }
    }

    if (!SymTabHeader && !DynSymTabHeader) {
      LogMan::Msg::I("No Symbol table");
      return;
    }

    uint64_t NumSymTabSymbols = 0;
    uint64_t NumDynSymSymbols = 0;
    if (SymTabHeader) {
      LOGMAN_THROW_A(SymTabHeader->sh_link < SectionHeaders.size(),
                       "Symbol table string table section is wrong");
      LOGMAN_THROW_A(SymTabHeader->sh_entsize == sizeof(Elf32_Sym),
                       "Entry size doesn't match symbol entry");

      StringTableHeader = SectionHeaders.at(SymTabHeader->sh_link)._32;
      StrTab = &RawFile.at(StringTableHeader->sh_offset);
      NumSymTabSymbols = SymTabHeader->sh_size / SymTabHeader->sh_entsize;
    }

    if (DynSymTabHeader) {
      LOGMAN_THROW_A(DynSymTabHeader->sh_link < SectionHeaders.size(),
                       "Symbol table string table section is wrong");
      LOGMAN_THROW_A(DynSymTabHeader->sh_entsize == sizeof(Elf32_Sym),
                       "Entry size doesn't match symbol entry");

      DynStringTableHeader = SectionHeaders.at(DynSymTabHeader->sh_link)._32;
      DynStrTab = &RawFile.at(DynStringTableHeader->sh_offset);
      NumDynSymSymbols = DynSymTabHeader->sh_size / DynSymTabHeader->sh_entsize;
    }

    uint64_t NumSymbols = NumSymTabSymbols + NumDynSymSymbols;

    Symbols.resize(NumSymbols);
    for (uint64_t i = 0; i < NumSymTabSymbols; ++i) {
      uint64_t offset = SymTabHeader->sh_offset + i * SymTabHeader->sh_entsize;
      Elf32_Sym const *Symbol =
          reinterpret_cast<Elf32_Sym const *>(&RawFile.at(offset));
      if (ELF32_ST_VISIBILITY(Symbol->st_other) != STV_HIDDEN &&
          Symbol->st_value != 0) {
        char const * Name = &StrTab[Symbol->st_name];
        if (Name[0] != '\0') {
          ELFSymbol *DefinedSymbol = &Symbols.at(i);
          DefinedSymbol->FileOffset = offset;
          DefinedSymbol->Address = Symbol->st_value;
          DefinedSymbol->Size = Symbol->st_size;
          DefinedSymbol->Type = ELF32_ST_TYPE(Symbol->st_info);
          DefinedSymbol->Bind = ELF32_ST_BIND(Symbol->st_info);
          DefinedSymbol->Name = Name;
          DefinedSymbol->SectionIndex = Symbol->st_shndx;

          SymbolMap[DefinedSymbol->Name] = DefinedSymbol;
          SymbolMapByAddress[DefinedSymbol->Address] = DefinedSymbol;
        }
      }
    }

    for (uint64_t i = 0; i < NumDynSymSymbols; ++i) {
      uint64_t offset = DynSymTabHeader->sh_offset + i * DynSymTabHeader->sh_entsize;
      Elf32_Sym const *Symbol =
          reinterpret_cast<Elf32_Sym const *>(&RawFile.at(offset));
      if (ELF32_ST_VISIBILITY(Symbol->st_other) != STV_HIDDEN &&
          Symbol->st_value != 0) {
        char const * Name = &DynStrTab[Symbol->st_name];
        if (Name[0] != '\0') {
          ELFSymbol *DefinedSymbol = &Symbols.at(NumSymTabSymbols + i);
          DefinedSymbol->FileOffset = offset;
          DefinedSymbol->Address = Symbol->st_value;
          DefinedSymbol->Size = Symbol->st_size;
          DefinedSymbol->Type = ELF32_ST_TYPE(Symbol->st_info);
          DefinedSymbol->Bind = ELF32_ST_BIND(Symbol->st_info);
          DefinedSymbol->Name = Name;
          DefinedSymbol->SectionIndex = Symbol->st_shndx;

          SymbolMap[DefinedSymbol->Name] = DefinedSymbol;
          SymbolMapByAddress[DefinedSymbol->Address] = DefinedSymbol;
        }
      }
    }
  }
  else {
    Elf64_Shdr const *SymTabHeader{nullptr};
    Elf64_Shdr const *StringTableHeader{nullptr};
    char const *StrTab{nullptr};

    Elf64_Shdr const *DynSymTabHeader{nullptr};
    Elf64_Shdr const *DynStringTableHeader{nullptr};
    char const *DynStrTab{nullptr};

    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf64_Shdr const *hdr = SectionHeaders.at(i)._64;
      if (hdr->sh_type == SHT_SYMTAB) {
        SymTabHeader = hdr;
        break;
      }
    }

    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf64_Shdr const *hdr = SectionHeaders.at(i)._64;
      if (hdr->sh_type == SHT_DYNSYM) {
        DynSymTabHeader = hdr;
        break;
      }
    }

    if (!SymTabHeader && !DynSymTabHeader) {
      LogMan::Msg::I("No Symbol table");
      return;
    }

    uint64_t NumSymTabSymbols = 0;
    uint64_t NumDynSymSymbols = 0;
    if (SymTabHeader) {
      LOGMAN_THROW_A(SymTabHeader->sh_link < SectionHeaders.size(),
                       "Symbol table string table section is wrong");
      LOGMAN_THROW_A(SymTabHeader->sh_entsize == sizeof(Elf64_Sym),
                       "Entry size doesn't match symbol entry");

      StringTableHeader = SectionHeaders.at(SymTabHeader->sh_link)._64;
      StrTab = &RawFile.at(StringTableHeader->sh_offset);
      NumSymTabSymbols = SymTabHeader->sh_size / SymTabHeader->sh_entsize;
    }

    if (DynSymTabHeader) {
      LOGMAN_THROW_A(DynSymTabHeader->sh_link < SectionHeaders.size(),
                       "Symbol table string table section is wrong");
      LOGMAN_THROW_A(DynSymTabHeader->sh_entsize == sizeof(Elf64_Sym),
                       "Entry size doesn't match symbol entry");

      DynStringTableHeader = SectionHeaders.at(DynSymTabHeader->sh_link)._64;
      DynStrTab = &RawFile.at(DynStringTableHeader->sh_offset);
      NumDynSymSymbols = DynSymTabHeader->sh_size / DynSymTabHeader->sh_entsize;
    }

    uint64_t NumSymbols = NumSymTabSymbols + NumDynSymSymbols;

    Symbols.resize(NumSymbols);
    for (uint64_t i = 0; i < NumSymTabSymbols; ++i) {
      uint64_t offset = SymTabHeader->sh_offset + i * SymTabHeader->sh_entsize;
      Elf64_Sym const *Symbol =
          reinterpret_cast<Elf64_Sym const *>(&RawFile.at(offset));
      if (ELF64_ST_VISIBILITY(Symbol->st_other) != STV_HIDDEN &&
          Symbol->st_value != 0) {
        char const * Name = &StrTab[Symbol->st_name];
        if (Name[0] != '\0') {
          ELFSymbol *DefinedSymbol = &Symbols.at(i);
          DefinedSymbol->FileOffset = offset;
          DefinedSymbol->Address = Symbol->st_value;
          DefinedSymbol->Size = Symbol->st_size;
          DefinedSymbol->Type = ELF64_ST_TYPE(Symbol->st_info);
          DefinedSymbol->Bind = ELF64_ST_BIND(Symbol->st_info);
          DefinedSymbol->Name = Name;
          DefinedSymbol->SectionIndex = Symbol->st_shndx;

          SymbolMap[DefinedSymbol->Name] = DefinedSymbol;
          SymbolMapByAddress[DefinedSymbol->Address] = DefinedSymbol;
        }
      }
    }

    for (uint64_t i = 0; i < NumDynSymSymbols; ++i) {
      uint64_t offset = DynSymTabHeader->sh_offset + i * DynSymTabHeader->sh_entsize;
      Elf64_Sym const *Symbol =
          reinterpret_cast<Elf64_Sym const *>(&RawFile.at(offset));
      if (ELF64_ST_VISIBILITY(Symbol->st_other) != STV_HIDDEN &&
          Symbol->st_value != 0) {
        char const * Name = &DynStrTab[Symbol->st_name];
        if (Name[0] != '\0') {
          ELFSymbol *DefinedSymbol = &Symbols.at(NumSymTabSymbols + i);
          DefinedSymbol->FileOffset = offset;
          DefinedSymbol->Address = Symbol->st_value;
          DefinedSymbol->Size = Symbol->st_size;
          DefinedSymbol->Type = ELF64_ST_TYPE(Symbol->st_info);
          DefinedSymbol->Bind = ELF64_ST_BIND(Symbol->st_info);
          DefinedSymbol->Name = Name;
          DefinedSymbol->SectionIndex = Symbol->st_shndx;

          SymbolMap[DefinedSymbol->Name] = DefinedSymbol;
          SymbolMapByAddress[DefinedSymbol->Address] = DefinedSymbol;
        }
      }
    }

    Elf64_Shdr const *StrHeader = SectionHeaders.at(Header._64.e_shstrndx)._64;
    char const *SHStrings = &RawFile.at(StrHeader->sh_offset);
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf64_Shdr const *hdr = SectionHeaders.at(i)._64;
      if (strcmp(&SHStrings[hdr->sh_name], ".eh_frame_hdr") == 0) {
        auto eh_frame_hdr = &RawFile.at(hdr->sh_offset);
        printf("Found it! %p\n", eh_frame_hdr);
        if (eh_frame_hdr[0] == 1 && eh_frame_hdr[1] == 0x1B && eh_frame_hdr[2] == 0x3 && eh_frame_hdr[3] == 0x3b) {
          // ptr enc : 4 bytes, signed, pcrel
          // fde count : 4 bytes udata
          // table enc : 4 bytes, signed, datarel
          printf("ptrenc: %d\n", *(int*)(eh_frame_hdr + 4));
          int fde_count = *(int*)(eh_frame_hdr + 8);
          printf("fde count: %d\n", fde_count);

          struct entry {
            int32_t pc;
            int32_t fde;
          };

          entry *Table = (entry*)(eh_frame_hdr+12);
          for (int f = 0; f < fde_count; f++) {
            uintptr_t Entry = (uintptr_t)(Table[f].pc + hdr->sh_offset);
            ELFSymbol sym;
            sym.Address = Entry;
            sym.Name = "unwind";
            sym.FileOffset = 1;
            Symbols.push_back(sym);
          }
        }
        break;
      }
    }
  }
}

void ELFContainer::GetDynamicLibs() {
  if (Mode == MODE_32BIT) {
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf32_Shdr const *hdr = SectionHeaders.at(i)._32;
      if (hdr->sh_type == SHT_DYNAMIC) {
        Elf32_Shdr const *StrHeader = SectionHeaders.at(hdr->sh_link)._32;
        char const *SHStrings = &RawFile.at(StrHeader->sh_offset);

        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; i < Entries; ++j) {
          Elf32_Dyn const *Dynamic = reinterpret_cast<Elf32_Dyn const*>(&RawFile.at(hdr->sh_offset + j * hdr->sh_entsize));
          if (Dynamic->d_tag == DT_NULL) break;
          if (Dynamic->d_tag == DT_NEEDED) {
            NecessaryLibs.emplace_back(&SHStrings[Dynamic->d_un.d_val]);
          }
        }
      }
    }
  }
  else {
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf64_Shdr const *hdr = SectionHeaders.at(i)._64;
      if (hdr->sh_type == SHT_DYNAMIC) {
        Elf64_Shdr const *StrHeader = SectionHeaders.at(hdr->sh_link)._64;
        char const *SHStrings = &RawFile.at(StrHeader->sh_offset);

        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; i < Entries; ++j) {
          Elf64_Dyn const *Dynamic = reinterpret_cast<Elf64_Dyn const*>(&RawFile.at(hdr->sh_offset + j * hdr->sh_entsize));
          if (Dynamic->d_tag == DT_NULL) break;
          if (Dynamic->d_tag == DT_NEEDED) {
            NecessaryLibs.emplace_back(&SHStrings[Dynamic->d_un.d_val]);
          }
        }
      }
    }
  }
}

void ELFContainer::AddSymbols(SymbolAdder Adder) {
  for (auto &Sym : Symbols) {
    if (Sym.FileOffset) {
      Adder(&Sym);
    }
  }
}

void ELFContainer::PrintHeader() const {
  if (Mode == MODE_32BIT) {
    LogMan::Msg::I("Type: %d", Header._32.e_type);
    LogMan::Msg::I("Machine: %d", Header._32.e_machine);
    LogMan::Msg::I("Version: %d", Header._32.e_version);
    LogMan::Msg::I("Entry point: 0x%lx", Header._32.e_entry);
    LogMan::Msg::I("PH Off: %d", Header._32.e_phoff);
    LogMan::Msg::I("SH Off: %d", Header._32.e_shoff);
    LogMan::Msg::I("Flags: %d", Header._32.e_flags);
    LogMan::Msg::I("EH Size: %d", Header._32.e_ehsize);
    LogMan::Msg::I("PH Num: %d", Header._32.e_phnum);
    LogMan::Msg::I("SH Num: %d", Header._32.e_shnum);
    LogMan::Msg::I("PH Entry Size: %d", Header._32.e_phentsize);
    LogMan::Msg::I("SH Entry Size: %d", Header._32.e_shentsize);
    LogMan::Msg::I("SH Str Index: %d", Header._32.e_shstrndx);
  }
  else {
    LogMan::Msg::I("Type: %d", Header._64.e_type);
    LogMan::Msg::I("Machine: %d", Header._64.e_machine);
    LogMan::Msg::I("Version: %d", Header._64.e_version);
    LogMan::Msg::I("Entry point: 0x%lx", Header._64.e_entry);
    LogMan::Msg::I("PH Off: %d", Header._64.e_phoff);
    LogMan::Msg::I("SH Off: %d", Header._64.e_shoff);
    LogMan::Msg::I("Flags: %d", Header._64.e_flags);
    LogMan::Msg::I("EH Size: %d", Header._64.e_ehsize);
    LogMan::Msg::I("PH Num: %d", Header._64.e_phnum);
    LogMan::Msg::I("SH Num: %d", Header._64.e_shnum);
    LogMan::Msg::I("PH Entry Size: %d", Header._64.e_phentsize);
    LogMan::Msg::I("SH Entry Size: %d", Header._64.e_shentsize);
    LogMan::Msg::I("SH Str Index: %d", Header._64.e_shstrndx);
  }
}

void ELFContainer::PrintSectionHeaders() const {
  if (Mode == MODE_32BIT) {
    LOGMAN_THROW_A(Header._32.e_shstrndx < SectionHeaders.size(),
                     "String index section is wrong index!");
    Elf32_Shdr const *StrHeader = SectionHeaders.at(Header._32.e_shstrndx)._32;
    char const *SHStrings = &RawFile.at(StrHeader->sh_offset);
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf32_Shdr const *hdr = SectionHeaders.at(i)._32;
      LogMan::Msg::I("Index: %d", i);
      LogMan::Msg::I("Name:       %s", &SHStrings[hdr->sh_name]);
      LogMan::Msg::I("Type:       %d", hdr->sh_type);
      LogMan::Msg::I("Flags:      %d", hdr->sh_flags);
      LogMan::Msg::I("Addr:       0x%lx", hdr->sh_addr);
      LogMan::Msg::I("Offset:     0x%lx", hdr->sh_offset);
      LogMan::Msg::I("Size:       %d", hdr->sh_size);
      LogMan::Msg::I("Link:       %d", hdr->sh_link);
      LogMan::Msg::I("Info:       %d", hdr->sh_info);
      LogMan::Msg::I("AddrAlign:  %d", hdr->sh_addralign);
      LogMan::Msg::I("Entry Size: %d", hdr->sh_entsize);
    }
  }
  else {
    LOGMAN_THROW_A(Header._64.e_shstrndx < SectionHeaders.size(),
                     "String index section is wrong index!");
    Elf64_Shdr const *StrHeader = SectionHeaders.at(Header._64.e_shstrndx)._64;
    char const *SHStrings = &RawFile.at(StrHeader->sh_offset);
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf64_Shdr const *hdr = SectionHeaders.at(i)._64;
      LogMan::Msg::I("Index: %d", i);
      LogMan::Msg::I("Name:       %s", &SHStrings[hdr->sh_name]);
      LogMan::Msg::I("Type:       %d", hdr->sh_type);
      LogMan::Msg::I("Flags:      %d", hdr->sh_flags);
      LogMan::Msg::I("Addr:       0x%lx", hdr->sh_addr);
      LogMan::Msg::I("Offset:     0x%lx", hdr->sh_offset);
      LogMan::Msg::I("Size:       %d", hdr->sh_size);
      LogMan::Msg::I("Link:       %d", hdr->sh_link);
      LogMan::Msg::I("Info:       %d", hdr->sh_info);
      LogMan::Msg::I("AddrAlign:  %d", hdr->sh_addralign);
      LogMan::Msg::I("Entry Size: %d", hdr->sh_entsize);
    }
  }
}

void ELFContainer::PrintProgramHeaders() const {
  if (Mode == MODE_32BIT) {
    LOGMAN_THROW_A(Header._32.e_shstrndx < SectionHeaders.size(),
                     "String index section is wrong index!");
    for (uint32_t i = 0; i < ProgramHeaders.size(); ++i) {
      Elf32_Phdr const *hdr = ProgramHeaders.at(i)._32;
      LogMan::Msg::I("Type:    %d", hdr->p_type);
      LogMan::Msg::I("Flags:   %d", hdr->p_flags);
      LogMan::Msg::I("Offset:  %d", hdr->p_offset);
      LogMan::Msg::I("VAddr:   0x%lx", hdr->p_vaddr);
      LogMan::Msg::I("PAddr:   0x%lx", hdr->p_paddr);
      LogMan::Msg::I("FSize:   %d", hdr->p_filesz);
      LogMan::Msg::I("MemSize: %d", hdr->p_memsz);
      LogMan::Msg::I("Align:   %d", hdr->p_align);
    }
  }
  else {
    LOGMAN_THROW_A(Header._64.e_shstrndx < SectionHeaders.size(),
                     "String index section is wrong index!");
    for (uint32_t i = 0; i < ProgramHeaders.size(); ++i) {
      Elf64_Phdr const *hdr = ProgramHeaders.at(i)._64;
      LogMan::Msg::I("Type:    %d", hdr->p_type);
      LogMan::Msg::I("Flags:   %d", hdr->p_flags);
      LogMan::Msg::I("Offset:  %d", hdr->p_offset);
      LogMan::Msg::I("VAddr:   0x%lx", hdr->p_vaddr);
      LogMan::Msg::I("PAddr:   0x%lx", hdr->p_paddr);
      LogMan::Msg::I("FSize:   %d", hdr->p_filesz);
      LogMan::Msg::I("MemSize: %d", hdr->p_memsz);
      LogMan::Msg::I("Align:   %d", hdr->p_align);
    }
  }
}

void ELFContainer::PrintSymbolTable() const {
  if (Mode == MODE_32BIT) {
    // Find the symbol table
    Elf32_Shdr const *SymTabHeader{nullptr};
    Elf32_Shdr const *StringTableHeader{nullptr};
    char const *StrTab{nullptr};
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf32_Shdr const *hdr = SectionHeaders.at(i)._32;
      if (hdr->sh_type == SHT_SYMTAB) {
        SymTabHeader = hdr;
        break;
      }
    }
    if (!SymTabHeader) {
      LogMan::Msg::I("No Symbol table");
      return;
    }

    LOGMAN_THROW_A(SymTabHeader->sh_link < SectionHeaders.size(),
                     "Symbol table string table section is wrong");
    LOGMAN_THROW_A(SymTabHeader->sh_entsize == sizeof(Elf32_Sym),
                     "Entry size doesn't match symbol entry");

    StringTableHeader = SectionHeaders.at(SymTabHeader->sh_link)._32;
    StrTab = &RawFile.at(StringTableHeader->sh_offset);

    uint64_t NumSymbols = SymTabHeader->sh_size / SymTabHeader->sh_entsize;
    for (uint64_t i = 0; i < NumSymbols; ++i) {
      uint64_t offset = SymTabHeader->sh_offset + i * SymTabHeader->sh_entsize;
      Elf32_Sym const *Symbol =
          reinterpret_cast<Elf32_Sym const *>(&RawFile.at(offset));
      std::ostringstream Str{};
      Str << i << " : " << std::hex << Symbol->st_value << std::dec << " "
          << Symbol->st_size << " " << uint32_t(Symbol->st_info) << " "
          << uint32_t(Symbol->st_other) << " " << Symbol->st_shndx << " "
          << &StrTab[Symbol->st_name];
      LogMan::Msg::I("%s", Str.str().c_str());
    }
  }
  else {
    // Find the symbol table
    Elf64_Shdr const *SymTabHeader{nullptr};
    Elf64_Shdr const *StringTableHeader{nullptr};
    char const *StrTab{nullptr};
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf64_Shdr const *hdr = SectionHeaders.at(i)._64;
      if (hdr->sh_type == SHT_SYMTAB) {
        SymTabHeader = hdr;
        break;
      }
    }
    if (!SymTabHeader) {
      LogMan::Msg::I("No Symbol table");
      return;
    }

    LOGMAN_THROW_A(SymTabHeader->sh_link < SectionHeaders.size(),
                     "Symbol table string table section is wrong");
    LOGMAN_THROW_A(SymTabHeader->sh_entsize == sizeof(Elf64_Sym),
                     "Entry size doesn't match symbol entry");

    StringTableHeader = SectionHeaders.at(SymTabHeader->sh_link)._64;
    StrTab = &RawFile.at(StringTableHeader->sh_offset);

    uint64_t NumSymbols = SymTabHeader->sh_size / SymTabHeader->sh_entsize;
    for (uint64_t i = 0; i < NumSymbols; ++i) {
      uint64_t offset = SymTabHeader->sh_offset + i * SymTabHeader->sh_entsize;
      Elf64_Sym const *Symbol =
          reinterpret_cast<Elf64_Sym const *>(&RawFile.at(offset));
      std::ostringstream Str{};
      Str << i << " : " << std::hex << Symbol->st_value << std::dec << " "
          << Symbol->st_size << " " << uint32_t(Symbol->st_info) << " "
          << uint32_t(Symbol->st_other) << " " << Symbol->st_shndx << " "
          << &StrTab[Symbol->st_name];
      LogMan::Msg::I("%s", Str.str().c_str());
    }
  }
}

void ELFContainer::PrintRelocationTable() const {
  if (Mode == MODE_32BIT) {
  }
  else {
    Elf64_Shdr const *RelaHeader{nullptr};
    Elf64_Shdr const *GOTHeader {nullptr};
    Elf64_Shdr const *DynSymHeader {nullptr};

    Elf64_Shdr const *StrHeader = SectionHeaders.at(Header._64.e_shstrndx)._64;
    char const *SHStrings = &RawFile.at(StrHeader->sh_offset);

    Elf64_Shdr const *StringTableHeader{nullptr};
    char const *StrTab{nullptr};

    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf64_Shdr const *hdr = SectionHeaders.at(i)._64;
      if (hdr->sh_type == SHT_REL) {
        LogMan::Msg::D("Unhandled REL section");
      }
      else if (hdr->sh_type == SHT_RELA) {
        RelaHeader = hdr;
        LogMan::Msg::D("Relocation Section: '%s'", &SHStrings[RelaHeader->sh_name]);

        if (RelaHeader->sh_info != 0) {
          LOGMAN_THROW_A(RelaHeader->sh_info < SectionHeaders.size(), "Rela header pointers to invalid GOT header");
          GOTHeader = SectionHeaders.at(RelaHeader->sh_info)._64;
        }

        if (RelaHeader->sh_link != 0) {
          LOGMAN_THROW_A(RelaHeader->sh_link < SectionHeaders.size(), "Rela header pointers to invalid dyndym header");
          DynSymHeader = SectionHeaders.at(RelaHeader->sh_link)._64;

          StringTableHeader = SectionHeaders.at(DynSymHeader->sh_link)._64;
          StrTab = &RawFile.at(StringTableHeader->sh_offset);
        }

        size_t EntryCount = RelaHeader->sh_size / RelaHeader->sh_entsize;
        Elf64_Rela const *Entries = reinterpret_cast<Elf64_Rela const*>(&RawFile.at(RelaHeader->sh_offset));

        for (unsigned j = 0; j < EntryCount; ++j) {
          Elf64_Rela const *Entry = &Entries[j];
          uint32_t Sym = Entry->r_info >> 32;
          uint32_t Type = Entry->r_info & ~0U;
          LogMan::Msg::D("RELA Entry %d", j);
          LogMan::Msg::D("\toffset: 0x%lx", Entry->r_offset);
          LogMan::Msg::D("\tSym:    0x%lx", Sym);
          if (DynSymHeader && Sym != 0) {
            LOGMAN_THROW_A(DynSymHeader->sh_entsize == sizeof(Elf64_Sym), "Oops, entry size doesn't match");

            uint64_t offset = DynSymHeader->sh_offset + Sym * DynSymHeader->sh_entsize;
            Elf64_Sym const *Symbol =
                reinterpret_cast<Elf64_Sym const *>(&RawFile.at(offset));
            LogMan::Msg::D("\tSym Name: '%s'", &StrTab[Symbol->st_name]);
          }

          LogMan::Msg::D("\tType:   0x%lx", Type);
          LogMan::Msg::D("\tadded:  0x%lx", Entry->r_addend);
          if (Type == R_X86_64_IRELATIVE) { // 37/0x25
            LogMan::Msg::D("\tR_x86_64_IRELATIVE");
          }
          else if (Type == R_X86_64_64) {
            LogMan::Msg::D("\tR_X86_64_64");
          }
          else if (Type == R_X86_64_RELATIVE) {
            LogMan::Msg::D("\tR_X86_64_RELATIVE");
          }
          else if (Type == R_X86_64_GLOB_DAT) {
            LogMan::Msg::D("\tR_X86_64_GLOB_DAT");
          }
          else if (Type == R_X86_64_JUMP_SLOT) {
            LogMan::Msg::D("\tR_X86_64_JUMP_SLOT");
          }
          else if (Type == R_X86_64_DTPMOD64) {
            LogMan::Msg::D("\tR_X86_64_DTPMOD64");
          }
          else if (Type == R_X86_64_DTPOFF64) {
            LogMan::Msg::D("\tR_X86_64_DTPOFF64");
          }
          else if (Type == R_X86_64_TPOFF64) {
            LogMan::Msg::D("\tR_X86_64_TPOFF64");
          }
          else {
            LogMan::Msg::D("Unknown relocation type: %d(0x%lx)", Type, Type);
          }
        }
      }
    }
  }
}

void ELFContainer::FixupRelocations(void *ELFBase, uint64_t GuestELFBase, SymbolGetter Getter) {
  if (Mode == MODE_32BIT) {
  }
  else {
    Elf64_Shdr const *RelaHeader{nullptr};
    Elf64_Shdr const *GOTHeader {nullptr};
    Elf64_Shdr const *DynSymHeader {nullptr};

    Elf64_Shdr const *StringTableHeader{nullptr};
    char const *StrTab{nullptr};

    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf64_Shdr const *hdr = SectionHeaders.at(i)._64;
      if (hdr->sh_type == SHT_REL) {
        LogMan::Msg::D("Unhandled REL section");
      }
      else if (hdr->sh_type == SHT_RELA) {
        RelaHeader = hdr;

        if (RelaHeader->sh_info != 0) {
          LOGMAN_THROW_A(RelaHeader->sh_info < SectionHeaders.size(), "Rela header pointers to invalid GOT header");
          GOTHeader = SectionHeaders.at(RelaHeader->sh_info)._64;
        }

        if (RelaHeader->sh_link != 0) {
          LOGMAN_THROW_A(RelaHeader->sh_link < SectionHeaders.size(), "Rela header pointers to invalid dyndym header");
          DynSymHeader = SectionHeaders.at(RelaHeader->sh_link)._64;

          StringTableHeader = SectionHeaders.at(DynSymHeader->sh_link)._64;
          StrTab = &RawFile.at(StringTableHeader->sh_offset);
        }

        size_t EntryCount = RelaHeader->sh_size / RelaHeader->sh_entsize;
        Elf64_Rela const *Entries = reinterpret_cast<Elf64_Rela const*>(&RawFile.at(RelaHeader->sh_offset));

        for (unsigned j = 0; j < EntryCount; ++j) {
          Elf64_Rela const *Entry = &Entries[j];
          uint32_t Sym = Entry->r_info >> 32;
          uint32_t Type = Entry->r_info & ~0U;
          Elf64_Sym const *EntrySymbol {nullptr};
          char const *EntrySymbolName {nullptr};
          if (DynSymHeader && Sym != 0) {
            LOGMAN_THROW_A(DynSymHeader->sh_entsize == sizeof(Elf64_Sym), "Oops, entry size doesn't match");

            uint64_t offset = DynSymHeader->sh_offset + Sym * DynSymHeader->sh_entsize;
            EntrySymbol =
                reinterpret_cast<Elf64_Sym const *>(&RawFile.at(offset));
            EntrySymbolName = &StrTab[EntrySymbol->st_name];
          }

          if (Type == R_X86_64_IRELATIVE) { // 37/0x25
            // Indirect (B + A)
            uint64_t *Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
            *Location = GuestELFBase + Entry->r_addend;
          }
          else if (Type == R_X86_64_64) {
            // S + A
            uint64_t *Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
            if (EntrySymbol != nullptr) {
              auto ELFSym = Getter(EntrySymbolName, 0);
              if (ELFSym != nullptr) {
                *Location = ELFSym->Address + Entry->r_addend;
              }
              else {
                *Location = 0xDEADBEEFBAD0DAD2ULL;
              }
            }
            else {
              *Location = 0xDEADBEEFBAD0DAD2ULL;
            }
          }
          else if (Type == R_X86_64_RELATIVE) {
            // B + A
            uint64_t *Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
            *Location = GuestELFBase + Entry->r_addend;
          }
          else if (Type == R_X86_64_GLOB_DAT) {
            // XXX: This is way wrong
            // S
            uint64_t *Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
            if (EntrySymbol != nullptr) {
              auto ELFSym = Getter(EntrySymbolName, 2); // Leave out Symbols from the main executable and only grab non-weak

              if (!ELFSym) {
                ELFSym = Getter(EntrySymbolName, 0);
              }
              if (!ELFSym) {
                ELFSym = Getter(EntrySymbolName, 3);
              }

              if (ELFSym != nullptr) {
                *Location = ELFSym->Address;
              }
              else {
                // XXX: This seems to be a loader edge case that if the symbol doesn't exist
                // and it is a weakly defined GLOB_DAT type then it is allowed to continue?
                // If we set Location to a value then apps crash
              }
            }
            else {
              *Location = 0xDEADBEEFBAD0DAD1ULL;
            }
          }
          else if (Type == R_X86_64_JUMP_SLOT) {
            // S
            uint64_t *Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
            if (EntrySymbol != nullptr) {
              auto ELFSym = Getter(EntrySymbolName, 0);
              if (!ELFSym) { // XXX: Try again
                ELFSym = Getter(EntrySymbolName, 3);
              }

              if (ELFSym != nullptr) {
                *Location = ELFSym->Address;
              }
              else {
                // XXX: This seems to be a loader edge case that if the symbol doesn't exist
                // and it is a weakly defined GLOB_DAT type then it is allowed to continue?
                *Location = 0xDEADBEEFBAD0DAD5ULL;
              }
            }
            else {
              *Location = 0xDEADBEEFBAD0DAD4ULL;
            }
          }
          else if (Type == R_X86_64_DTPMOD64) {
            // XXX: This is supposed to be the ID of the module that the symbol comes from for TLS purposes?
            uint64_t *Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
            *Location = 0;
          }
          else if (Type == R_X86_64_DTPOFF64) {
            uint64_t *Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
            if (EntrySymbol != nullptr) {
              *Location = EntrySymbol->st_value + Entry->r_addend;
            }
            else {
              *Location = 0xDEADBEEFBAD0DAD6ULL;
            }
          }
          else if (Type == R_X86_64_TPOFF64) {
            uint64_t *Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
            if (EntrySymbol != nullptr) {
              // XXX: This is supposed to be a symbol with a TLS offset?
              *Location = EntrySymbol->st_value + Entry->r_addend;
            }
            else {
              // If we set Location to a value then apps crash
              // *Location = 0xDEADBEEFBAD0DAD3ULL;
              LogMan::Msg::D("TPOFF without Entry? %lx + %lx + %lx", GuestELFBase, TLSHeader._64->p_paddr, Entry->r_addend);
              if (1) {
                *Location = TLSHeader._64->p_paddr + Entry->r_addend;
              }
              else if (Entry->r_offset == 0x1e3dc8) {
                *Location = 0xDEADBEEFBAD0DAD8ULL;
              }
              else {
                *Location = Entry->r_addend - 0xb00'0;
              }
            }
          }
          else {
            LogMan::Msg::D("Unknown relocation type: %d(0x%lx)", Type, Type);
          }
        }
      }
    }
  }
}

void ELFContainer::PrintInitArray() const {
  if (Mode == MODE_32BIT) {
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf32_Shdr const *hdr = SectionHeaders.at(i)._32;
      if (hdr->sh_type == SHT_INIT_ARRAY) {
        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; j < Entries; ++j) {
          LogMan::Msg::D("init_array[%d]", j);
          LogMan::Msg::D("\t%p", *reinterpret_cast<uint64_t const*>(&RawFile.at(hdr->sh_offset+ j * hdr->sh_entsize)));
        }
      }
    }
  }
  else {
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf64_Shdr const *hdr = SectionHeaders.at(i)._64;
      if (hdr->sh_type == SHT_INIT_ARRAY) {
        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; j < Entries; ++j) {
          LogMan::Msg::D("init_array[%d]", j);
          LogMan::Msg::D("\t%p", *reinterpret_cast<uint64_t const*>(&RawFile.at(hdr->sh_offset+ j * hdr->sh_entsize)));
        }
      }
    }
  }
}

void ELFContainer::PrintDynamicTable() const {
  if (Mode == MODE_32BIT) {
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf32_Shdr const *hdr = SectionHeaders.at(i)._32;
      if (hdr->sh_type == SHT_DYNAMIC) {
        Elf32_Shdr const *StrHeader = SectionHeaders.at(hdr->sh_link)._32;
        char const *SHStrings = &RawFile.at(StrHeader->sh_offset);

        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; i < Entries; ++j) {
          Elf32_Dyn const *Dynamic = reinterpret_cast<Elf32_Dyn const*>(&RawFile.at(hdr->sh_offset + j * hdr->sh_entsize));
#define PRINT(x, y, z) x (Dynamic->d_tag == DT_##y ) LogMan::Msg::D("Dyn %d: (" #y ") 0x%lx", j, Dynamic->d_un.z);
          if (Dynamic->d_tag == DT_NULL) {
            break;
          }
          else if (Dynamic->d_tag == DT_NEEDED) {
            LogMan::Msg::D("Dyn %d: (NEEDED) '%s'", j, &SHStrings[Dynamic->d_un.d_val]);
          }
          else if (Dynamic->d_tag == DT_SONAME) {
            LogMan::Msg::D("Dyn %d: (SONAME) '%s'", j, &SHStrings[Dynamic->d_un.d_val]);
          }
          PRINT(else if, HASH, d_val)
          PRINT(else if, INIT, d_val)
          PRINT(else if, FINI, d_val)
          PRINT(else if, INIT_ARRAY, d_val)
          PRINT(else if, INIT_ARRAYSZ, d_val)
          PRINT(else if, FINI_ARRAY, d_val)
          PRINT(else if, FINI_ARRAYSZ, d_val)
          PRINT(else if, GNU_HASH, d_val)
          PRINT(else if, STRTAB, d_val)
          PRINT(else if, SYMTAB, d_val)
          PRINT(else if, STRSZ, d_val)
          PRINT(else if, SYMENT, d_val)
          PRINT(else if, DEBUG, d_val)
          PRINT(else if, PLTGOT, d_val)
          PRINT(else if, PLTRELSZ, d_val)
          PRINT(else if, PLTREL, d_val)
          PRINT(else if, JMPREL, d_val)
          PRINT(else if, RELA, d_val)
          PRINT(else if, RELASZ, d_val)
          PRINT(else if, RELAENT, d_val)
          PRINT(else if, VERNEED, d_val)
          PRINT(else if, VERNEEDNUM, d_val)
          PRINT(else if, VERSYM, d_val)
          else if (Dynamic->d_tag >= DT_LOOS && Dynamic->d_tag <= DT_HIOS) {
            LogMan::Msg::D("Dyn %d: (OSSpecific) 0x%lx", j, Dynamic->d_tag);
          }
          else if (Dynamic->d_tag >= DT_LOPROC && Dynamic->d_tag <= DT_HIPROC) {
            LogMan::Msg::D("Dyn %d: (Proc-Specific) 0x%lx", j, Dynamic->d_tag);
          }
          PRINT(else if, RELACOUNT, d_val)
          PRINT(else if, RELCOUNT, d_val)
          PRINT(else if, VERDEF, d_val)
          PRINT(else if, VERDEFNUM, d_val)
          PRINT(else if, FLAGS, d_val)
          else
            LogMan::Msg::D("Unknown dynamic section: %d(0x%lx)", Dynamic->d_tag, Dynamic->d_tag);
#undef PRINT
        }
      }
    }
  }
  else {
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf64_Shdr const *hdr = SectionHeaders.at(i)._64;
      if (hdr->sh_type == SHT_DYNAMIC) {
        Elf64_Shdr const *StrHeader = SectionHeaders.at(hdr->sh_link)._64;
        char const *SHStrings = &RawFile.at(StrHeader->sh_offset);

        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; i < Entries; ++j) {
          Elf64_Dyn const *Dynamic = reinterpret_cast<Elf64_Dyn const*>(&RawFile.at(hdr->sh_offset + j * hdr->sh_entsize));
#define PRINT(x, y, z) x (Dynamic->d_tag == DT_##y ) LogMan::Msg::D("Dyn %d: (" #y ") 0x%lx", j, Dynamic->d_un.z);
          if (Dynamic->d_tag == DT_NULL) {
            break;
          }
          else if (Dynamic->d_tag == DT_NEEDED) {
            LogMan::Msg::D("Dyn %d: (NEEDED) '%s'", j, &SHStrings[Dynamic->d_un.d_val]);
          }
          else if (Dynamic->d_tag == DT_SONAME) {
            LogMan::Msg::D("Dyn %d: (SONAME) '%s'", j, &SHStrings[Dynamic->d_un.d_val]);
          }
          PRINT(else if, HASH, d_val)
          PRINT(else if, INIT, d_val)
          PRINT(else if, FINI, d_val)
          PRINT(else if, INIT_ARRAY, d_val)
          PRINT(else if, INIT_ARRAYSZ, d_val)
          PRINT(else if, FINI_ARRAY, d_val)
          PRINT(else if, FINI_ARRAYSZ, d_val)
          PRINT(else if, GNU_HASH, d_val)
          PRINT(else if, STRTAB, d_val)
          PRINT(else if, SYMTAB, d_val)
          PRINT(else if, STRSZ, d_val)
          PRINT(else if, SYMENT, d_val)
          PRINT(else if, DEBUG, d_val)
          PRINT(else if, PLTGOT, d_val)
          PRINT(else if, PLTRELSZ, d_val)
          PRINT(else if, PLTREL, d_val)
          PRINT(else if, JMPREL, d_val)
          PRINT(else if, RELA, d_val)
          PRINT(else if, RELASZ, d_val)
          PRINT(else if, RELAENT, d_val)
          PRINT(else if, VERNEED, d_val)
          PRINT(else if, VERNEEDNUM, d_val)
          PRINT(else if, VERSYM, d_val)
          else if (Dynamic->d_tag >= DT_LOOS && Dynamic->d_tag <= DT_HIOS) {
            LogMan::Msg::D("Dyn %d: (OSSpecific) 0x%lx", j, Dynamic->d_tag);
          }
          else if (Dynamic->d_tag >= DT_LOPROC && Dynamic->d_tag <= DT_HIPROC) {
            LogMan::Msg::D("Dyn %d: (Proc-Specific) 0x%lx", j, Dynamic->d_tag);
          }
          PRINT(else if, RELACOUNT, d_val)
          PRINT(else if, RELCOUNT, d_val)
          PRINT(else if, VERDEF, d_val)
          PRINT(else if, VERDEFNUM, d_val)
          PRINT(else if, FLAGS, d_val)
          else
            LogMan::Msg::D("Unknown dynamic section: %d(0x%lx)", Dynamic->d_tag, Dynamic->d_tag);
#undef PRINT
        }
      }
    }
  }
}

void ELFContainer::GetInitLocations(uint64_t GuestELFBase, std::vector<uint64_t> *Locations) {
  if (Mode == MODE_32BIT) {
    // If INIT exists then add that first
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf32_Shdr const *hdr = SectionHeaders.at(i)._32;
      if (hdr->sh_type == SHT_DYNAMIC) {
        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; i < Entries; ++j) {
          Elf32_Dyn const *Dynamic = reinterpret_cast<Elf32_Dyn const*>(&RawFile.at(hdr->sh_offset + j * hdr->sh_entsize));
          if (Dynamic->d_tag == DT_NULL) break;
          if (Dynamic->d_tag == DT_INIT) {
            Locations->emplace_back(GuestELFBase + Dynamic->d_un.d_val);
          }
        }
      }
    }

    // Fill init_array
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf32_Shdr const *hdr = SectionHeaders.at(i)._32;
      if (hdr->sh_type == SHT_INIT_ARRAY) {
        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; j < Entries; ++j) {
          Locations->emplace_back(GuestELFBase + *reinterpret_cast<uint64_t const*>(&RawFile.at(hdr->sh_offset+ j * hdr->sh_entsize)));
        }
      }
    }
  }
  else {
    // If INIT exists then add that first
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf64_Shdr const *hdr = SectionHeaders.at(i)._64;
      if (hdr->sh_type == SHT_DYNAMIC) {
        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; i < Entries; ++j) {
          Elf64_Dyn const *Dynamic = reinterpret_cast<Elf64_Dyn const*>(&RawFile.at(hdr->sh_offset + j * hdr->sh_entsize));
          if (Dynamic->d_tag == DT_NULL) break;
          if (Dynamic->d_tag == DT_INIT) {
            Locations->emplace_back(GuestELFBase + Dynamic->d_un.d_val);
          }
        }
      }
    }

    // Fill init_array
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      Elf64_Shdr const *hdr = SectionHeaders.at(i)._64;
      if (hdr->sh_type == SHT_INIT_ARRAY) {
        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; j < Entries; ++j) {
          Locations->emplace_back(GuestELFBase + *reinterpret_cast<uint64_t const*>(&RawFile.at(hdr->sh_offset+ j * hdr->sh_entsize)));
        }
      }
    }
  }
}

} // namespace ELFLoader
