// SPDX-License-Identifier: MIT
/*
$info$
tags: glue|elf-parsing
desc: Loads and parses an elf to memory. Also handles some loading & logic.
$end_info$
*/

#include "Linux/Utils/ELFContainer.h"
#include <FEXCore/Utils/FileLoading.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/Filesystem.h>
#include <FEXHeaderUtils/SymlinkChecks.h>

#include <algorithm>
#include <cstring>
#include <elf.h>
#include <fcntl.h>
#include <memory>
#include <linux/limits.h>
#include <system_error>
#include <sys/stat.h>
#include <unistd.h>

namespace ELFLoader {

static ELFContainer::ELFType CheckELFType(uint8_t* Data) {
  if (Data[EI_MAG0] != ELFMAG0 || Data[EI_MAG1] != ELFMAG1 || Data[EI_MAG2] != ELFMAG2 || Data[EI_MAG3] != ELFMAG3) {
    return ELFContainer::ELFType::TYPE_NONE;
  }

  if (Data[EI_CLASS] == ELFCLASS32) {
    Elf32_Ehdr* Header = reinterpret_cast<Elf32_Ehdr*>(Data);
    if (Header->e_machine == EM_386) {
      return ELFContainer::ELFType::TYPE_X86_32;
    }
  } else if (Data[EI_CLASS] == ELFCLASS64) {
    Elf64_Ehdr* Header = reinterpret_cast<Elf64_Ehdr*>(Data);
    if (Header->e_machine == EM_X86_64) {
      return ELFContainer::ELFType::TYPE_X86_64;
    }
  }

  return ELFContainer::ELFType::TYPE_OTHER_ELF;
}

ELFContainer::ELFType ELFContainer::GetELFType(const fextl::string& Filename) {
  // Open the Filename to determine if it is a shebang file.
  int FD = open(Filename.c_str(), O_RDONLY | O_CLOEXEC);
  if (FD == -1) {
    return ELFType::TYPE_NONE;
  }

  auto ELFType = GetELFType(FD);
  close(FD);
  return ELFType;
}

ELFContainer::ELFType ELFContainer::GetELFType(int FD) {
  // We don't know the state of the FD coming in since this might be a guest tracked FD.
  // Need to be extra careful here not to adjust file offsets and status flags.
  //
  // We can't use dup since that makes the FD have the same underlying state backing both FDs.

  // We need to first determine the file size through fstat.
  struct stat buf {};
  if (fstat(FD, &buf) == -1) {
    // Couldn't get size.
    return ELFType::TYPE_NONE;
  }

  constexpr size_t ELFHeaderSize = std::max(sizeof(Elf32_Ehdr), sizeof(Elf64_Ehdr));
  if (buf.st_size < ELFHeaderSize) {
    // Is not a valid ELF.
    return ELFType::TYPE_NONE;
  }

  std::array<char, ELFHeaderSize> RawFile;

  // Read the header so we can tell if it is a supported ELF file.
  // Can't adjust file offset, so use pread.
  if (pread(FD, RawFile.data(), RawFile.size(), 0) != RawFile.size()) {
    // Couldn't read
    LogMan::Msg::EFmt("Couldn't read potential ELF FD");
    return ELFType::TYPE_NONE;
  }

  return CheckELFType(reinterpret_cast<uint8_t*>(RawFile.data()));
}

ELFContainer::ELFContainer(const fextl::string& Filename, const fextl::string& RootFS, bool CustomInterpreter) {
  Loaded = true;
  if (!LoadELF(Filename)) {
    LogMan::Msg::EFmt("Couldn't Load ELF file");
    Loaded = false;
    return;
  }

  if (InterpreterHeader._64 && !CustomInterpreter) {
    // If we we are dynamic application then we have an interpreter program header
    // We need to load that ELF instead if it exists
    // We are no longer dynamic since we are executing the interpreter
    const char* RawString {};
    if (Mode == MODE_32BIT) {
      RawString = &RawFile.at(InterpreterHeader._32->p_offset);
    } else {
      RawString = &RawFile.at(InterpreterHeader._64->p_offset);
    }
    fextl::string RootFSLink = RootFS + RawString;
    char Filename[PATH_MAX];
    while (FHU::Symlinks::IsSymlink(RootFSLink)) {
      // Do some special handling if the RootFS's linker is a symlink
      // Ubuntu's rootFS by default provides an absolute location symlink to the linker
      // Resolve this around back to the rootfs
      const auto SymlinkTarget = FHU::Symlinks::ResolveSymlink(RootFSLink, Filename);
      if (FHU::Filesystem::IsAbsolute(SymlinkTarget)) {
        RootFSLink = RootFS;
        RootFSLink += SymlinkTarget;
      } else {
        break;
      }
    }
    if (LoadELF(RootFSLink)) {
      // Found the interpreter in the rootfs
    } else if (!LoadELF(RawString)) {
      LogMan::Msg::EFmt("Failed to find guest ELF's interpter '{}'", RawString);
      LogMan::Msg::EFmt("Did you forget to set an x86 rootfs? Currently '{}'", RootFS);
      Loaded = false;
      return;
    }
  } else if (InterpreterHeader._64) {
    GetDynamicLibs();
  }


  CalculateMemoryLayouts();
  CalculateSymbols();
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

bool ELFContainer::LoadELF(const fextl::string& Filename) {
  if (!FEXCore::FileLoading::LoadFile(RawFile, Filename)) {
    return false;
  }

  InterpreterHeader._64 = nullptr;

  SectionHeaders.clear();
  ProgramHeaders.clear();

  uint8_t* Ident = reinterpret_cast<uint8_t*>(RawFile.data());

  if (Ident[EI_MAG0] != ELFMAG0 || Ident[EI_MAG1] != ELFMAG1 || Ident[EI_MAG2] != ELFMAG2 || Ident[EI_MAG3] != ELFMAG3) {
    LogMan::Msg::EFmt("ELF missing magic cookie");
    return false;
  }

  if (Ident[EI_CLASS] == ELFCLASS32) {
    return LoadELF_32();
  } else if (Ident[EI_CLASS] == ELFCLASS64) {
    return LoadELF_64();
  }

  LogMan::Msg::EFmt("Unknown ELF type");
  return false;
}

bool ELFContainer::LoadELF_32() {
  Mode = MODE_32BIT;

  memcpy(&Header, reinterpret_cast<Elf32_Ehdr*>(RawFile.data()), sizeof(Elf32_Ehdr));
  LOGMAN_THROW_A_FMT(Header._32.e_phentsize == sizeof(Elf32_Phdr), "PH Entry size wasn't correct size");
  LOGMAN_THROW_A_FMT(Header._32.e_shentsize == sizeof(Elf32_Shdr), "PH Entry size wasn't correct size");

  if (Header._32.e_machine != EM_386) {
    LogMan::Msg::DFmt("32bit ELF wasn't x86 based");
    return false;
  }

  SectionHeaders.resize(Header._32.e_shnum);
  ProgramHeaders.resize(Header._32.e_phnum);

  Elf32_Shdr* RawShdrs = reinterpret_cast<Elf32_Shdr*>(&RawFile.at(Header._32.e_shoff));
  Elf32_Phdr* RawPhdrs = reinterpret_cast<Elf32_Phdr*>(&RawFile.at(Header._32.e_phoff));

  for (uint32_t i = 0; i < Header._32.e_shnum; ++i) {
    SectionHeaders[i]._32 = &RawShdrs[i];
  }

  for (uint32_t i = 0; i < Header._32.e_phnum; ++i) {
    ProgramHeaders[i]._32 = &RawPhdrs[i];
    if (ProgramHeaders[i]._32->p_type == PT_INTERP) {
      InterpreterHeader = ProgramHeaders[i];
      DynamicLinker = reinterpret_cast<const char*>(&RawFile.at(InterpreterHeader._32->p_offset));
    }
  }

  DynamicProgram = Header._32.e_type != ET_EXEC;

  // Default BRK size
  BRKSize = 4096;

  return true;
}

bool ELFContainer::LoadELF_64() {
  Mode = MODE_64BIT;

  memcpy(&Header, reinterpret_cast<Elf64_Ehdr*>(RawFile.data()), sizeof(Elf64_Ehdr));
  LOGMAN_THROW_A_FMT(Header._64.e_phentsize == 56, "PH Entry size wasn't 56");
  LOGMAN_THROW_A_FMT(Header._64.e_shentsize == 64, "PH Entry size wasn't 64");

  if (Header._64.e_machine != EM_X86_64) {
    LogMan::Msg::DFmt("64bit ELF wasn't x86-64 based");
    return false;
  }

  SectionHeaders.resize(Header._64.e_shnum);
  ProgramHeaders.resize(Header._64.e_phnum);

  Elf64_Shdr* RawShdrs = reinterpret_cast<Elf64_Shdr*>(&RawFile.at(Header._64.e_shoff));
  Elf64_Phdr* RawPhdrs = reinterpret_cast<Elf64_Phdr*>(&RawFile.at(Header._64.e_phoff));

  for (uint32_t i = 0; i < Header._64.e_shnum; ++i) {
    SectionHeaders[i]._64 = &RawShdrs[i];
  }

  for (uint32_t i = 0; i < Header._64.e_phnum; ++i) {
    ProgramHeaders[i]._64 = &RawPhdrs[i];
    if (ProgramHeaders[i]._64->p_type == PT_INTERP) {
      InterpreterHeader = ProgramHeaders[i];
      DynamicLinker = reinterpret_cast<const char*>(&RawFile.at(InterpreterHeader._64->p_offset));
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
      const Elf32_Phdr* hdr = ProgramHeaders.at(i)._32;
      if (hdr->p_type == PT_LOAD) {
        // LogMan::Msg::DFmt("PT_LOAD: Base: {} Offset: [0x{:x}, 0x{:x})", Offset, hdr->p_paddr, hdr->p_filesz);
        Writer(&RawFile.at(hdr->p_offset), Offset + hdr->p_paddr, hdr->p_filesz);
      }

      if (hdr->p_type == PT_TLS) {
        Writer(&RawFile.at(hdr->p_offset), Offset + hdr->p_paddr, hdr->p_filesz);
      }
    }
  } else {
    for (uint32_t i = 0; i < ProgramHeaders.size(); ++i) {
      const Elf64_Phdr* hdr = ProgramHeaders.at(i)._64;
      if (hdr->p_type == PT_LOAD) {
        Writer(&RawFile.at(hdr->p_offset), Offset + hdr->p_paddr, hdr->p_filesz);
      }

      if (hdr->p_type == PT_TLS) {
        Writer(&RawFile.at(hdr->p_offset), Offset + hdr->p_paddr, hdr->p_filesz);
      }
    }
  }
}

const ELFSymbol* ELFContainer::GetSymbol(const char* Name) {
  auto Sym = SymbolMap.find(Name);
  if (Sym == SymbolMap.end()) {
    return nullptr;
  }
  return Sym->second;
}
const ELFSymbol* ELFContainer::GetSymbol(uint64_t Address) {
  auto Sym = SymbolMapByAddress.find(Address);
  if (Sym == SymbolMapByAddress.end()) {
    return nullptr;
  }
  return Sym->second;
}
const ELFSymbol* ELFContainer::GetSymbolInRange(RangeType Address) {
  auto Sym = SymbolMapByAddress.upper_bound(Address.first);
  if (Sym != SymbolMapByAddress.begin()) {
    --Sym;
  }
  if (Sym == SymbolMapByAddress.end()) {
    return nullptr;
  }

  if ((Sym->second->Address + Sym->second->Size) < Address.first) {
    return nullptr;
  }

  return Sym->second;
}

void ELFContainer::CalculateMemoryLayouts() {
  uint64_t MinPhysAddr = ~0ULL;
  uint64_t MaxPhysAddr = 0;
  uint64_t PhysMemSize = 0;

  if (Mode == MODE_32BIT) {
    for (uint32_t i = 0; i < ProgramHeaders.size(); ++i) {
      Elf32_Phdr* hdr = ProgramHeaders.at(i)._32;
      if (hdr->p_memsz > 0) {
        MinPhysAddr = std::min(MinPhysAddr, static_cast<uint64_t>(hdr->p_paddr));
        MaxPhysAddr = std::max(MaxPhysAddr, static_cast<uint64_t>(hdr->p_paddr) + hdr->p_memsz);
      }
      if (hdr->p_type == PT_TLS) {
        TLSHeader._32 = hdr;
      }
    }
  } else {
    for (uint32_t i = 0; i < ProgramHeaders.size(); ++i) {
      Elf64_Phdr* hdr = ProgramHeaders.at(i)._64;

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
  MaxPhysAddr = FEXCore::AlignUp(MaxPhysAddr, 4096);
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
    const Elf32_Shdr* SymTabHeader {nullptr};
    const Elf32_Shdr* StringTableHeader {nullptr};
    const char* StrTab {nullptr};

    const Elf32_Shdr* DynSymTabHeader {nullptr};
    const Elf32_Shdr* DynStringTableHeader {nullptr};
    const char* DynStrTab {nullptr};

    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      const Elf32_Shdr* hdr = SectionHeaders.at(i)._32;
      if (hdr->sh_type == SHT_SYMTAB) {
        SymTabHeader = hdr;
        break;
      }
    }

    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      const Elf32_Shdr* hdr = SectionHeaders.at(i)._32;
      if (hdr->sh_type == SHT_DYNSYM) {
        DynSymTabHeader = hdr;
        break;
      }
    }

    if (!SymTabHeader && !DynSymTabHeader) {
      LogMan::Msg::IFmt("No Symbol table");
      return;
    }

    uint64_t NumSymTabSymbols = 0;
    uint64_t NumDynSymSymbols = 0;
    if (SymTabHeader) {
      LOGMAN_THROW_A_FMT(SymTabHeader->sh_link < SectionHeaders.size(), "Symbol table string table section is wrong");
      LOGMAN_THROW_A_FMT(SymTabHeader->sh_entsize == sizeof(Elf32_Sym), "Entry size doesn't match symbol entry");

      StringTableHeader = SectionHeaders.at(SymTabHeader->sh_link)._32;
      StrTab = &RawFile.at(StringTableHeader->sh_offset);
      NumSymTabSymbols = SymTabHeader->sh_size / SymTabHeader->sh_entsize;
    }

    if (DynSymTabHeader) {
      LOGMAN_THROW_A_FMT(DynSymTabHeader->sh_link < SectionHeaders.size(), "Symbol table string table section is wrong");
      LOGMAN_THROW_A_FMT(DynSymTabHeader->sh_entsize == sizeof(Elf32_Sym), "Entry size doesn't match symbol entry");

      DynStringTableHeader = SectionHeaders.at(DynSymTabHeader->sh_link)._32;
      DynStrTab = &RawFile.at(DynStringTableHeader->sh_offset);
      NumDynSymSymbols = DynSymTabHeader->sh_size / DynSymTabHeader->sh_entsize;
    }

    uint64_t NumSymbols = NumSymTabSymbols + NumDynSymSymbols;

    Symbols.resize(NumSymbols);
    for (uint64_t i = 0; i < NumSymTabSymbols; ++i) {
      uint64_t offset = SymTabHeader->sh_offset + i * SymTabHeader->sh_entsize;
      const Elf32_Sym* Symbol = reinterpret_cast<const Elf32_Sym*>(&RawFile.at(offset));
      if (ELF32_ST_VISIBILITY(Symbol->st_other) != STV_HIDDEN && Symbol->st_value != 0) {
        const char* Name = &StrTab[Symbol->st_name];
        if (Name[0] != '\0') {
          ELFSymbol* DefinedSymbol = &Symbols.at(i);
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
      const Elf32_Sym* Symbol = reinterpret_cast<const Elf32_Sym*>(&RawFile.at(offset));
      if (ELF32_ST_VISIBILITY(Symbol->st_other) != STV_HIDDEN && Symbol->st_value != 0) {
        const char* Name = &DynStrTab[Symbol->st_name];
        if (Name[0] != '\0') {
          ELFSymbol* DefinedSymbol = &Symbols.at(NumSymTabSymbols + i);
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

    const Elf32_Shdr* StrHeader = SectionHeaders.at(Header._32.e_shstrndx)._32;
    const char* SHStrings = &RawFile.at(StrHeader->sh_offset);
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      const Elf32_Shdr* hdr = SectionHeaders.at(i)._32;
      if (strcmp(&SHStrings[hdr->sh_name], ".eh_frame_hdr") == 0) {
        auto eh_frame_hdr = &RawFile.at(hdr->sh_offset);
        // we only handle this specific unwind table encoding
        if (eh_frame_hdr[0] == 1 && eh_frame_hdr[1] == 0x1B && eh_frame_hdr[2] == 0x3 && eh_frame_hdr[3] == 0x3b) {
          // ptr enc : 4 bytes, signed, pcrel
          // fde count : 4 bytes udata
          // table enc : 4 bytes, signed, datarel
          int fde_count = *(int*)(eh_frame_hdr + 8);
          UnwindEntries.clear();
          UnwindEntries.reserve(fde_count);

          struct entry {
            int32_t pc;
            int32_t fde;
          };

          entry* Table = (entry*)(eh_frame_hdr + 12);
          for (int f = 0; f < fde_count; f++) {
            uintptr_t Entry = (uintptr_t)(Table[f].pc + hdr->sh_offset);
            UnwindEntries.push_back(Entry);
          }
        }
        break;
      }
    }
  } else {
    const Elf64_Shdr* SymTabHeader {nullptr};
    const Elf64_Shdr* StringTableHeader {nullptr};
    const char* StrTab {nullptr};

    const Elf64_Shdr* DynSymTabHeader {nullptr};
    const Elf64_Shdr* DynStringTableHeader {nullptr};
    const char* DynStrTab {nullptr};

    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      const Elf64_Shdr* hdr = SectionHeaders.at(i)._64;
      if (hdr->sh_type == SHT_SYMTAB) {
        SymTabHeader = hdr;
        break;
      }
    }

    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      const Elf64_Shdr* hdr = SectionHeaders.at(i)._64;
      if (hdr->sh_type == SHT_DYNSYM) {
        DynSymTabHeader = hdr;
        break;
      }
    }

    if (!SymTabHeader && !DynSymTabHeader) {
      LogMan::Msg::IFmt("No Symbol table");
      return;
    }

    uint64_t NumSymTabSymbols = 0;
    uint64_t NumDynSymSymbols = 0;
    if (SymTabHeader) {
      LOGMAN_THROW_A_FMT(SymTabHeader->sh_link < SectionHeaders.size(), "Symbol table string table section is wrong");
      LOGMAN_THROW_A_FMT(SymTabHeader->sh_entsize == sizeof(Elf64_Sym), "Entry size doesn't match symbol entry");

      StringTableHeader = SectionHeaders.at(SymTabHeader->sh_link)._64;
      StrTab = &RawFile.at(StringTableHeader->sh_offset);
      NumSymTabSymbols = SymTabHeader->sh_size / SymTabHeader->sh_entsize;
    }

    if (DynSymTabHeader) {
      LOGMAN_THROW_A_FMT(DynSymTabHeader->sh_link < SectionHeaders.size(), "Symbol table string table section is wrong");
      LOGMAN_THROW_A_FMT(DynSymTabHeader->sh_entsize == sizeof(Elf64_Sym), "Entry size doesn't match symbol entry");

      DynStringTableHeader = SectionHeaders.at(DynSymTabHeader->sh_link)._64;
      DynStrTab = &RawFile.at(DynStringTableHeader->sh_offset);
      NumDynSymSymbols = DynSymTabHeader->sh_size / DynSymTabHeader->sh_entsize;
    }

    uint64_t NumSymbols = NumSymTabSymbols + NumDynSymSymbols;

    Symbols.resize(NumSymbols);
    for (uint64_t i = 0; i < NumSymTabSymbols; ++i) {
      uint64_t offset = SymTabHeader->sh_offset + i * SymTabHeader->sh_entsize;
      const Elf64_Sym* Symbol = reinterpret_cast<const Elf64_Sym*>(&RawFile.at(offset));
      if (ELF64_ST_VISIBILITY(Symbol->st_other) != STV_HIDDEN && Symbol->st_value != 0) {
        const char* Name = &StrTab[Symbol->st_name];
        if (Name[0] != '\0') {
          ELFSymbol* DefinedSymbol = &Symbols.at(i);
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
      const Elf64_Sym* Symbol = reinterpret_cast<const Elf64_Sym*>(&RawFile.at(offset));
      if (ELF64_ST_VISIBILITY(Symbol->st_other) != STV_HIDDEN && Symbol->st_value != 0) {
        const char* Name = &DynStrTab[Symbol->st_name];
        if (Name[0] != '\0') {
          ELFSymbol* DefinedSymbol = &Symbols.at(NumSymTabSymbols + i);
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

    const Elf64_Shdr* StrHeader = SectionHeaders.at(Header._64.e_shstrndx)._64;
    const char* SHStrings = &RawFile.at(StrHeader->sh_offset);
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      const Elf64_Shdr* hdr = SectionHeaders.at(i)._64;
      if (strcmp(&SHStrings[hdr->sh_name], ".eh_frame_hdr") == 0) {
        auto eh_frame_hdr = &RawFile.at(hdr->sh_offset);
        // we only handle this specific unwind table encoding
        if (eh_frame_hdr[0] == 1 && eh_frame_hdr[1] == 0x1B && eh_frame_hdr[2] == 0x3 && eh_frame_hdr[3] == 0x3b) {
          // ptr enc : 4 bytes, signed, pcrel
          // fde count : 4 bytes udata
          // table enc : 4 bytes, signed, datarel
          int fde_count = *(int*)(eh_frame_hdr + 8);
          UnwindEntries.clear();
          UnwindEntries.reserve(fde_count);

          struct entry {
            int32_t pc;
            int32_t fde;
          };

          entry* Table = (entry*)(eh_frame_hdr + 12);
          for (int f = 0; f < fde_count; f++) {
            uintptr_t Entry = (uintptr_t)(Table[f].pc + hdr->sh_offset);
            UnwindEntries.push_back(Entry);
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
      const Elf32_Shdr* hdr = SectionHeaders.at(i)._32;
      if (hdr->sh_type == SHT_DYNAMIC) {
        const Elf32_Shdr* StrHeader = SectionHeaders.at(hdr->sh_link)._32;
        const char* SHStrings = &RawFile.at(StrHeader->sh_offset);

        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; i < Entries; ++j) {
          const Elf32_Dyn* Dynamic = reinterpret_cast<const Elf32_Dyn*>(&RawFile.at(hdr->sh_offset + j * hdr->sh_entsize));
          if (Dynamic->d_tag == DT_NULL) {
            break;
          }
          if (Dynamic->d_tag == DT_NEEDED) {
            NecessaryLibs.emplace_back(&SHStrings[Dynamic->d_un.d_val]);
          }
        }
      }
    }
  } else {
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      const Elf64_Shdr* hdr = SectionHeaders.at(i)._64;
      if (hdr->sh_type == SHT_DYNAMIC) {
        const Elf64_Shdr* StrHeader = SectionHeaders.at(hdr->sh_link)._64;
        const char* SHStrings = &RawFile.at(StrHeader->sh_offset);

        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; i < Entries; ++j) {
          const Elf64_Dyn* Dynamic = reinterpret_cast<const Elf64_Dyn*>(&RawFile.at(hdr->sh_offset + j * hdr->sh_entsize));
          if (Dynamic->d_tag == DT_NULL) {
            break;
          }
          if (Dynamic->d_tag == DT_NEEDED) {
            NecessaryLibs.emplace_back(&SHStrings[Dynamic->d_un.d_val]);
          }
        }
      }
    }
  }
}

void ELFContainer::AddSymbols(SymbolAdder Adder) {
  for (auto& Sym : Symbols) {
    if (Sym.FileOffset) {
      Adder(&Sym);
    }
  }
}
void ELFContainer::AddUnwindEntries(UnwindAdder Adder) {
  for (auto Entry : UnwindEntries) {
    Adder(Entry);
  }
}

void ELFContainer::FixupRelocations(void* ELFBase, uint64_t GuestELFBase, SymbolGetter Getter) {
  if (Mode == MODE_32BIT) {
  } else {
    const Elf64_Shdr* RelaHeader {nullptr};
    const Elf64_Shdr* DynSymHeader {nullptr};

    const Elf64_Shdr* StringTableHeader {nullptr};
    const char* StrTab {nullptr};

    for (size_t i = 0; i < SectionHeaders.size(); ++i) {
      const auto* hdr = SectionHeaders[i]._64;
      if (hdr->sh_type == SHT_REL) {
        LogMan::Msg::DFmt("Unhandled REL section");
      } else if (hdr->sh_type == SHT_RELA) {
        RelaHeader = hdr;

        if (RelaHeader->sh_info != 0) {
          LOGMAN_THROW_A_FMT(RelaHeader->sh_info < SectionHeaders.size(), "Rela header pointers to invalid GOT header");
        }

        if (RelaHeader->sh_link != 0) {
          LOGMAN_THROW_A_FMT(RelaHeader->sh_link < SectionHeaders.size(), "Rela header pointers to invalid dyndym header");
          DynSymHeader = SectionHeaders.at(RelaHeader->sh_link)._64;

          StringTableHeader = SectionHeaders.at(DynSymHeader->sh_link)._64;
          StrTab = &RawFile.at(StringTableHeader->sh_offset);
        }

        const size_t EntryCount = RelaHeader->sh_size / RelaHeader->sh_entsize;
        const auto* Entries = reinterpret_cast<const Elf64_Rela*>(&RawFile.at(RelaHeader->sh_offset));

        for (size_t j = 0; j < EntryCount; ++j) {
          const auto* Entry = &Entries[j];
          const uint32_t Sym = Entry->r_info >> 32;
          const uint32_t Type = Entry->r_info & ~0U;
          const Elf64_Sym* EntrySymbol {nullptr};
          const char* EntrySymbolName {nullptr};
          if (DynSymHeader && Sym != 0) {
            LOGMAN_THROW_A_FMT(DynSymHeader->sh_entsize == sizeof(Elf64_Sym), "Oops, entry size doesn't match");

            const uint64_t offset = DynSymHeader->sh_offset + Sym * DynSymHeader->sh_entsize;
            EntrySymbol = reinterpret_cast<const Elf64_Sym*>(&RawFile.at(offset));
            EntrySymbolName = &StrTab[EntrySymbol->st_name];
          }

          if (Type == R_X86_64_IRELATIVE) { // 37/0x25
            // Indirect (B + A)
            uint64_t* Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
            *Location = GuestELFBase + Entry->r_addend;
          } else if (Type == R_X86_64_64) {
            // S + A
            uint64_t* Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
            if (EntrySymbol != nullptr) {
              auto ELFSym = Getter(EntrySymbolName, 0);
              if (ELFSym != nullptr) {
                *Location = ELFSym->Address + Entry->r_addend;
              } else {
                *Location = 0xDEADBEEFBAD0DAD2ULL;
              }
            } else {
              *Location = 0xDEADBEEFBAD0DAD2ULL;
            }
          } else if (Type == R_X86_64_RELATIVE) {
            // B + A
            uint64_t* Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
            *Location = GuestELFBase + Entry->r_addend;
          } else if (Type == R_X86_64_GLOB_DAT) {
            // XXX: This is way wrong
            // S
            uint64_t* Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
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
              } else {
                // XXX: This seems to be a loader edge case that if the symbol doesn't exist
                // and it is a weakly defined GLOB_DAT type then it is allowed to continue?
                // If we set Location to a value then apps crash
              }
            } else {
              *Location = 0xDEADBEEFBAD0DAD1ULL;
            }
          } else if (Type == R_X86_64_JUMP_SLOT) {
            // S
            uint64_t* Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
            if (EntrySymbol != nullptr) {
              auto ELFSym = Getter(EntrySymbolName, 0);
              if (!ELFSym) { // XXX: Try again
                ELFSym = Getter(EntrySymbolName, 3);
              }

              if (ELFSym != nullptr) {
                *Location = ELFSym->Address;
              } else {
                // XXX: This seems to be a loader edge case that if the symbol doesn't exist
                // and it is a weakly defined GLOB_DAT type then it is allowed to continue?
                *Location = 0xDEADBEEFBAD0DAD5ULL;
              }
            } else {
              *Location = 0xDEADBEEFBAD0DAD4ULL;
            }
          } else if (Type == R_X86_64_DTPMOD64) {
            // XXX: This is supposed to be the ID of the module that the symbol comes from for TLS purposes?
            uint64_t* Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
            *Location = 0;
          } else if (Type == R_X86_64_DTPOFF64) {
            uint64_t* Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
            if (EntrySymbol != nullptr) {
              *Location = EntrySymbol->st_value + Entry->r_addend;
            } else {
              *Location = 0xDEADBEEFBAD0DAD6ULL;
            }
          } else if (Type == R_X86_64_TPOFF64) {
            uint64_t* Location = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(ELFBase) + Entry->r_offset);
            if (EntrySymbol != nullptr) {
              // XXX: This is supposed to be a symbol with a TLS offset?
              *Location = EntrySymbol->st_value + Entry->r_addend;
            } else {
              // If we set Location to a value then apps crash
              // *Location = 0xDEADBEEFBAD0DAD3ULL;
              LogMan::Msg::DFmt("TPOFF without Entry? {:x} + {:x} + {:x}", GuestELFBase, TLSHeader._64->p_paddr, Entry->r_addend);
              if (1) {
                *Location = TLSHeader._64->p_paddr + Entry->r_addend;
              } else if (Entry->r_offset == 0x1e3dc8) {
                *Location = 0xDEADBEEFBAD0DAD8ULL;
              } else {
                *Location = Entry->r_addend - 0xb00'0;
              }
            }
          } else {
            LogMan::Msg::DFmt("Unknown relocation type: {}(0x{:x})", Type, Type);
          }
        }
      }
    }
  }
}

void ELFContainer::GetInitLocations(uint64_t GuestELFBase, fextl::vector<uint64_t>* Locations) {
  if (Mode == MODE_32BIT) {
    // If INIT exists then add that first
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      const Elf32_Shdr* hdr = SectionHeaders.at(i)._32;
      if (hdr->sh_type == SHT_DYNAMIC) {
        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; i < Entries; ++j) {
          const Elf32_Dyn* Dynamic = reinterpret_cast<const Elf32_Dyn*>(&RawFile.at(hdr->sh_offset + j * hdr->sh_entsize));
          if (Dynamic->d_tag == DT_NULL) {
            break;
          }
          if (Dynamic->d_tag == DT_INIT) {
            Locations->emplace_back(GuestELFBase + Dynamic->d_un.d_val);
          }
        }
      }
    }

    // Fill init_array
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      const Elf32_Shdr* hdr = SectionHeaders.at(i)._32;
      if (hdr->sh_type == SHT_INIT_ARRAY) {
        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; j < Entries; ++j) {
          Locations->emplace_back(GuestELFBase + *reinterpret_cast<const uint64_t*>(&RawFile.at(hdr->sh_offset + j * hdr->sh_entsize)));
        }
      }
    }
  } else {
    // If INIT exists then add that first
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      const Elf64_Shdr* hdr = SectionHeaders.at(i)._64;
      if (hdr->sh_type == SHT_DYNAMIC) {
        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; i < Entries; ++j) {
          const Elf64_Dyn* Dynamic = reinterpret_cast<const Elf64_Dyn*>(&RawFile.at(hdr->sh_offset + j * hdr->sh_entsize));
          if (Dynamic->d_tag == DT_NULL) {
            break;
          }
          if (Dynamic->d_tag == DT_INIT) {
            Locations->emplace_back(GuestELFBase + Dynamic->d_un.d_val);
          }
        }
      }
    }

    // Fill init_array
    for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
      const Elf64_Shdr* hdr = SectionHeaders.at(i)._64;
      if (hdr->sh_type == SHT_INIT_ARRAY) {
        size_t Entries = hdr->sh_size / hdr->sh_entsize;
        for (size_t j = 0; j < Entries; ++j) {
          Locations->emplace_back(GuestELFBase + *reinterpret_cast<const uint64_t*>(&RawFile.at(hdr->sh_offset + j * hdr->sh_entsize)));
        }
      }
    }
  }
}

} // namespace ELFLoader
