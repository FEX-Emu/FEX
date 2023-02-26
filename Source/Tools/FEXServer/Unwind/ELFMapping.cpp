#include <FEXCore/Utils/LogManager.h>

#include "ELFMapping.h"
#include "FileMapping.h"
#include <cassert>
#include <cstring>
#include <elf.h>
#include <map>
#include <sys/mman.h>
#include <sys/stat.h>

namespace ELFMapping {
  uint64_t IsELFBits(ELFMemMapping *Mapping) {
    constexpr size_t ELFHeaderSize = std::max(sizeof(Elf32_Ehdr), sizeof(Elf64_Ehdr));
    if (Mapping->TotalSize < ELFHeaderSize) {
      return 0;
    }

    const uint8_t* Data = reinterpret_cast<const uint8_t*>(Mapping->BaseRemapped);

    if (Data[EI_MAG0] != ELFMAG0 ||
        Data[EI_MAG1] != ELFMAG1 ||
        Data[EI_MAG2] != ELFMAG2 ||
        Data[EI_MAG3] != ELFMAG3) {
      return 0;
    }

    if (Data[EI_CLASS] == ELFCLASS32) {
      const Elf32_Ehdr *Header = reinterpret_cast<const Elf32_Ehdr *>(Data);
      if (Header->e_machine == EM_386) {
        return 32;
      }
    }
    else if (Data[EI_CLASS] == ELFCLASS64) {
      const Elf64_Ehdr *Header = reinterpret_cast<const Elf64_Ehdr *>(Data);
      if (Header->e_machine == EM_X86_64) {
        return 64;
      }
    }

    return 0;
  }

  void ParseELFSymbols64(ELFMemMapping *Mapping) {
    const char* RawBase = reinterpret_cast<const char*>(Mapping->BaseRemapped);
    memcpy(&Mapping->Header, RawBase, sizeof(Elf64_Ehdr));
    Mapping->SectionHeaders.resize(Mapping->Header._64.e_shnum);
    Mapping->ProgramHeaders.resize(Mapping->Header._64.e_phnum);

    const Elf64_Shdr *RawShdrs =
      reinterpret_cast<const Elf64_Shdr *>(&RawBase[Mapping->Header._64.e_shoff]);
    const Elf64_Phdr *RawPhdrs =
      reinterpret_cast<const Elf64_Phdr *>(&RawBase[Mapping->Header._64.e_phoff]);

    if (Mapping->Header._64.e_shoff > Mapping->TotalSize) {
      // Corrupt input.
      return;
    }
    for (uint32_t i = 0; i < Mapping->Header._64.e_shnum; ++i) {
      Mapping->SectionHeaders[i]._64 = &RawShdrs[i];
    }

    for (uint32_t i = 0; i < Mapping->Header._64.e_phnum; ++i) {
      Mapping->ProgramHeaders[i]._64 = &RawPhdrs[i];
    }

    // Calculate symbols
    Elf64_Shdr const *SymTabHeader{nullptr};
    Elf64_Shdr const *StringTableHeader{nullptr};
    char const *StrTab{nullptr};

    Elf64_Shdr const *DynSymTabHeader{nullptr};
    Elf64_Shdr const *DynStringTableHeader{nullptr};
    char const *DynStrTab{nullptr};

    for (const auto &Header : Mapping->SectionHeaders) {
      Elf64_Shdr const *hdr = Header._64;
      if (hdr->sh_type == SHT_SYMTAB) {
        SymTabHeader = hdr;
      }
      if (hdr->sh_type == SHT_DYNSYM) {
        DynSymTabHeader = hdr;
      }
    }

    if (!SymTabHeader && !DynSymTabHeader) {
      return;
    }

    uint64_t NumSymTabSymbols = 0;
    uint64_t NumDynSymSymbols = 0;
    if (SymTabHeader) {
      StringTableHeader = Mapping->SectionHeaders.at(SymTabHeader->sh_link)._64;
      StrTab = &RawBase[StringTableHeader->sh_offset];
      NumSymTabSymbols = SymTabHeader->sh_size / SymTabHeader->sh_entsize;
    }

    if (DynSymTabHeader) {
      DynStringTableHeader = Mapping->SectionHeaders.at(DynSymTabHeader->sh_link)._64;
      DynStrTab = &RawBase[DynStringTableHeader->sh_offset];
      NumDynSymSymbols = DynSymTabHeader->sh_size / DynSymTabHeader->sh_entsize;
    }

    uint64_t NumSymbols = NumSymTabSymbols + NumDynSymSymbols;

    Mapping->Symbols.resize(NumSymbols);
    for (uint64_t i = 0; i < NumSymTabSymbols; ++i) {
      uint64_t offset = SymTabHeader->sh_offset + i * SymTabHeader->sh_entsize;
      Elf64_Sym const *Symbol =
          reinterpret_cast<Elf64_Sym const *>(&RawBase[offset]);
      if (Symbol->st_value != 0) {
        char const * Name = &StrTab[Symbol->st_name];
        if (Name[0] == '\0') {
          auto AddrName = Mapping->AddressSymbolNames.emplace_back("func_" + std::to_string((uint64_t)Mapping->OriginalBase + Symbol->st_value));
          Name = AddrName.data();
        }

        auto DefinedSymbol = &Mapping->Symbols.at(i);
        DefinedSymbol->FileOffset = offset;
        DefinedSymbol->Address = (uint64_t)Mapping->OriginalBase + Symbol->st_value;
        DefinedSymbol->Size = Symbol->st_size;
        DefinedSymbol->Type = ELF64_ST_TYPE(Symbol->st_info);
        DefinedSymbol->Bind = ELF64_ST_BIND(Symbol->st_info);
        DefinedSymbol->Name = Name;
        DefinedSymbol->SectionIndex = Symbol->st_shndx;

        Mapping->SymbolMap[DefinedSymbol->Name] = DefinedSymbol;
        Mapping->SymbolMapByAddress[DefinedSymbol->Address] = DefinedSymbol;
      }
    }

    for (uint64_t i = 0; i < NumDynSymSymbols; ++i) {
      uint64_t offset = DynSymTabHeader->sh_offset + i * DynSymTabHeader->sh_entsize;
      Elf64_Sym const *Symbol =
          reinterpret_cast<Elf64_Sym const *>(&RawBase[offset]);
      if (Symbol->st_value != 0) {
        char const * Name = &DynStrTab[Symbol->st_name];
        if (Name[0] == '\0') {
          auto AddrName = Mapping->AddressSymbolNames.emplace_back("func_" + std::to_string((uint64_t)Mapping->OriginalBase + Symbol->st_value));
          Name = AddrName.data();
        }

        auto DefinedSymbol = &Mapping->Symbols.at(NumSymTabSymbols + i);
        DefinedSymbol->FileOffset = offset;
        DefinedSymbol->Address = (uint64_t)Mapping->OriginalBase + Symbol->st_value;

        DefinedSymbol->Size = Symbol->st_size;
        DefinedSymbol->Type = ELF64_ST_TYPE(Symbol->st_info);
        DefinedSymbol->Bind = ELF64_ST_BIND(Symbol->st_info);
        DefinedSymbol->Name = Name;
        DefinedSymbol->SectionIndex = Symbol->st_shndx;
        Mapping->SymbolMap[DefinedSymbol->Name] = DefinedSymbol;
        Mapping->SymbolMapByAddress[DefinedSymbol->Address] = DefinedSymbol;
      }
    }

    Elf64_Shdr const *StrHeader = Mapping->SectionHeaders.at(Mapping->Header._64.e_shstrndx)._64;
    char const *SHStrings = &RawBase[StrHeader->sh_offset];
    for (uint32_t i = 0; i < Mapping->SectionHeaders.size(); ++i) {
      Elf64_Shdr const *hdr = Mapping->SectionHeaders.at(i)._64;
      if (strcmp(&SHStrings[hdr->sh_name], ".eh_frame_hdr") == 0) {
        auto eh_frame_hdr = &RawBase[hdr->sh_offset];
        // we only handle this specific unwind table encoding
        if (eh_frame_hdr[0] == 1 && eh_frame_hdr[1] == 0x1B && eh_frame_hdr[2] == 0x3 && eh_frame_hdr[3] == 0x3b) {
          // ptr enc : 4 bytes, signed, pcrel
          // fde count : 4 bytes udata
          // table enc : 4 bytes, signed, datarel
          int fde_count = *(int*)(eh_frame_hdr + 8);
          Mapping->UnwindEntries.clear();
          Mapping->UnwindEntries.reserve(fde_count);

          struct entry {
            int32_t pc;
            int32_t fde;
          };

          entry *Table = (entry*)(eh_frame_hdr+12);
          for (int f = 0; f < fde_count; f++) {
            uintptr_t Entry = (uintptr_t)(Table[f].pc + hdr->sh_offset);
            Mapping->UnwindEntries.push_back(Entry);
          }
        }
        break;
      }
    }

    // Invalid mapping for top end search.
    Mapping->SymbolMapByAddress[~0ULL] = {};
  }

  void ParseELFSymbols32(ELFMemMapping *Mapping) {
    const char* RawBase = reinterpret_cast<const char*>(Mapping->BaseRemapped);
    memcpy(&Mapping->Header, RawBase, sizeof(Elf32_Ehdr));
    Mapping->SectionHeaders.resize(Mapping->Header._32.e_shnum);
    Mapping->ProgramHeaders.resize(Mapping->Header._32.e_phnum);

    const Elf32_Shdr *RawShdrs =
      reinterpret_cast<const Elf32_Shdr *>(&RawBase[Mapping->Header._32.e_shoff]);
    const Elf32_Phdr *RawPhdrs =
      reinterpret_cast<const Elf32_Phdr *>(&RawBase[Mapping->Header._32.e_phoff]);

    if (Mapping->Header._32.e_shoff > Mapping->TotalSize) {
      // Corrupt input.
      return;
    }
    for (uint32_t i = 0; i < Mapping->Header._32.e_shnum; ++i) {
      Mapping->SectionHeaders[i]._32 = &RawShdrs[i];
    }

    for (uint32_t i = 0; i < Mapping->Header._32.e_phnum; ++i) {
      Mapping->ProgramHeaders[i]._32 = &RawPhdrs[i];
    }

    // Calculate symbols
    Elf32_Shdr const *SymTabHeader{nullptr};
    Elf32_Shdr const *StringTableHeader{nullptr};
    char const *StrTab{nullptr};

    Elf32_Shdr const *DynSymTabHeader{nullptr};
    Elf32_Shdr const *DynStringTableHeader{nullptr};
    char const *DynStrTab{nullptr};

    for (const auto &Header : Mapping->SectionHeaders) {
      Elf32_Shdr const *hdr = Header._32;
      if (hdr->sh_type == SHT_SYMTAB) {
        SymTabHeader = hdr;
      }
      if (hdr->sh_type == SHT_DYNSYM) {
        DynSymTabHeader = hdr;
      }
    }

    if (!SymTabHeader && !DynSymTabHeader) {
      return;
    }

    uint64_t NumSymTabSymbols = 0;
    uint64_t NumDynSymSymbols = 0;
    if (SymTabHeader) {
      StringTableHeader = Mapping->SectionHeaders.at(SymTabHeader->sh_link)._32;
      StrTab = &RawBase[StringTableHeader->sh_offset];
      NumSymTabSymbols = SymTabHeader->sh_size / SymTabHeader->sh_entsize;
    }

    if (DynSymTabHeader) {
      DynStringTableHeader = Mapping->SectionHeaders.at(DynSymTabHeader->sh_link)._32;
      DynStrTab = &RawBase[DynStringTableHeader->sh_offset];
      NumDynSymSymbols = DynSymTabHeader->sh_size / DynSymTabHeader->sh_entsize;
    }

    uint64_t NumSymbols = NumSymTabSymbols + NumDynSymSymbols;

    Mapping->Symbols.resize(NumSymbols);
    for (uint64_t i = 0; i < NumSymTabSymbols; ++i) {
      uint64_t offset = SymTabHeader->sh_offset + i * SymTabHeader->sh_entsize;
      Elf32_Sym const *Symbol =
          reinterpret_cast<Elf32_Sym const *>(&RawBase[offset]);
      if (Symbol->st_value != 0) {
        char const * Name = &StrTab[Symbol->st_name];
        if (Name[0] == '\0') {
          auto AddrName = Mapping->AddressSymbolNames.emplace_back("func_" + std::to_string((uint64_t)Mapping->OriginalBase + Symbol->st_value));
          Name = AddrName.data();
        }

        auto DefinedSymbol = &Mapping->Symbols.at(i);
        DefinedSymbol->FileOffset = offset;
        DefinedSymbol->Address = (uint64_t)Mapping->OriginalBase + Symbol->st_value;
        DefinedSymbol->Size = Symbol->st_size;
        DefinedSymbol->Type = ELF32_ST_TYPE(Symbol->st_info);
        DefinedSymbol->Bind = ELF32_ST_BIND(Symbol->st_info);
        DefinedSymbol->Name = Name;
        DefinedSymbol->SectionIndex = Symbol->st_shndx;

        Mapping->SymbolMap[DefinedSymbol->Name] = DefinedSymbol;
        Mapping->SymbolMapByAddress[DefinedSymbol->Address] = DefinedSymbol;
      }
    }

    for (uint64_t i = 0; i < NumDynSymSymbols; ++i) {
      uint64_t offset = DynSymTabHeader->sh_offset + i * DynSymTabHeader->sh_entsize;
      Elf32_Sym const *Symbol =
          reinterpret_cast<Elf32_Sym const *>(&RawBase[offset]);
      if (Symbol->st_value != 0) {
        char const * Name = &DynStrTab[Symbol->st_name];
        if (Name[0] == '\0') {
          auto AddrName = Mapping->AddressSymbolNames.emplace_back("func_" + std::to_string((uint64_t)Mapping->OriginalBase + Symbol->st_value));
          Name = AddrName.data();
        }

        auto DefinedSymbol = &Mapping->Symbols.at(NumSymTabSymbols + i);
        DefinedSymbol->FileOffset = offset;
        DefinedSymbol->Address = (uint64_t)Mapping->OriginalBase + Symbol->st_value;

        DefinedSymbol->Size = Symbol->st_size;
        DefinedSymbol->Type = ELF32_ST_TYPE(Symbol->st_info);
        DefinedSymbol->Bind = ELF32_ST_BIND(Symbol->st_info);
        DefinedSymbol->Name = Name;
        DefinedSymbol->SectionIndex = Symbol->st_shndx;
        Mapping->SymbolMap[DefinedSymbol->Name] = DefinedSymbol;
        Mapping->SymbolMapByAddress[DefinedSymbol->Address] = DefinedSymbol;
      }
    }

    Elf32_Shdr const *StrHeader = Mapping->SectionHeaders.at(Mapping->Header._32.e_shstrndx)._32;
    char const *SHStrings = &RawBase[StrHeader->sh_offset];
    for (uint32_t i = 0; i < Mapping->SectionHeaders.size(); ++i) {
      Elf32_Shdr const *hdr = Mapping->SectionHeaders.at(i)._32;
      if (strcmp(&SHStrings[hdr->sh_name], ".eh_frame_hdr") == 0) {
        auto eh_frame_hdr = &RawBase[hdr->sh_offset];
        // we only handle this specific unwind table encoding
        if (eh_frame_hdr[0] == 1 && eh_frame_hdr[1] == 0x1B && eh_frame_hdr[2] == 0x3 && eh_frame_hdr[3] == 0x3b) {
          // ptr enc : 4 bytes, signed, pcrel
          // fde count : 4 bytes udata
          // table enc : 4 bytes, signed, datarel
          int fde_count = *(int*)(eh_frame_hdr + 8);
          Mapping->UnwindEntries.clear();
          Mapping->UnwindEntries.reserve(fde_count);

          struct entry {
            int32_t pc;
            int32_t fde;
          };

          entry *Table = (entry*)(eh_frame_hdr+12);
          for (int f = 0; f < fde_count; f++) {
            uintptr_t Entry = (uintptr_t)(Table[f].pc + hdr->sh_offset);
            Mapping->UnwindEntries.push_back(Entry);
          }
        }
        break;
      }
    }

    // Invalid mapping for top end search.
    Mapping->SymbolMapByAddress[~0ULL] = {};
  }

  void ParseELFSymbols(ELFMemMapping *Mapping) {
    // Find the ELF header, which should be the first element.
    uint64_t Bits = IsELFBits(Mapping);
    if (Bits == 64) {
      ParseELFSymbols64(Mapping);
    }
    else if (Bits == 32) {
      ParseELFSymbols32(Mapping);
    }
  }

  ELFMemMapping *LoadELFMapping(const FileMapping::FileMapping *Mapping, int FD) {
    if (FD == -1) {
      return nullptr;
    }

    ELFMemMapping *RemapMapping = new ELFMemMapping{};

    RemapMapping->FD = FD;

    struct stat buf{};
    if (fstat(FD, &buf) != 0) {
        return nullptr;
    }

    RemapMapping->OriginalBase = Mapping->Begin;
    RemapMapping->TotalSize = buf.st_size;

    RemapMapping->BaseRemapped = reinterpret_cast<char*>(mmap(nullptr, RemapMapping->TotalSize, PROT_READ, MAP_SHARED, FD, 0));

    ParseELFSymbols(RemapMapping);

    return RemapMapping;
  }

  ELFSymbol *GetSymbolFromAddress(ELFMemMapping *Mapping, uint64_t Addr) {
    auto Sym = Mapping->SymbolMapByAddress.upper_bound(Addr);
    if (Sym != Mapping->SymbolMapByAddress.begin()) {
      --Sym;
    }

    if (Sym == Mapping->SymbolMapByAddress.end()) {
      return nullptr;
    }

    // <= necessary for single byte sized instructions, like thunks.
    if (Sym->second->Address <= Addr && Addr <= (Sym->second->Address + Sym->second->Size)) {
      return Sym->second;
    }
    return nullptr;
  }

  ELFMemMapping::~ELFMemMapping() {
    if (BaseRemapped) {
      // If the FD was mapped then make sure to unmap it.
      munmap(BaseRemapped, TotalSize);
    }

    // FD is tracked externally so we don't close it here.
  }
}
