#pragma once
#include <vector>
#include <string>
#include <elf.h>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>

#include "Linux/Utils/ELFContainer.h"

/*
  Simpler elf parser, checks for the elf MAGIC COOKIE
  and loads the phdrs
  Also keeps an fd open
*/

struct ELFParser {
  Elf64_Ehdr ehdr;
  std::vector<Elf64_Phdr> phdrs;
  ::ELFLoader::ELFContainer::ELFType type {::ELFLoader::ELFContainer::TYPE_NONE};

  std::string InterpreterElf;
  std::string BuildID;

  int fd {-1};
  bool OwnFd { false };

  static bool pread_all(int fd, void *buf, size_t nbytes, __off_t offset) { 
    return pread(fd, buf, nbytes, offset) == nbytes;
  }

  // This must not modify the file pointer or otherwise the state of the file
  bool ReadElf(int NewFD, bool Silent = true) {
    Closefd();
    static_assert(EI_CLASS == 4);

    fd = NewFD;
    type = ::ELFLoader::ELFContainer::TYPE_NONE;

    if (fd == -1) {
      // Likely just doesn't exist
      return false;
    }

    uint8_t header[5];
    if (!pread_all(fd, header, sizeof(header), 0)) {
      if (!Silent) {
        LogMan::Msg::EFmt("Failed to read elf header from '{}'", fd);
      }
      return false;
    }

    if (header[0] != ELFMAG0 || header[1] != ELFMAG1 || header[2] != ELFMAG2 || header[3] != ELFMAG3) {
      if (!Silent) {
        LogMan::Msg::EFmt("Elf header from '{}' doesn't match ELF MAGIC", fd);
      }
      return false;
    }

    type = ::ELFLoader::ELFContainer::TYPE_OTHER_ELF;

    if (header[EI_CLASS] == ELFCLASS32) {
      Elf32_Ehdr hdr32;
      if (!pread_all(fd, &hdr32, sizeof(hdr32), 0)) {
        if (!Silent) {
          LogMan::Msg::EFmt("Failed to read Ehdr32 from '{}'", fd);
        }
        return false;
      }

      // do the sizes match up as expected?

      // check elf header
      if (hdr32.e_ehsize != sizeof(hdr32)) {
        if (!Silent) {
          LogMan::Msg::EFmt("Invalid e_ehsize32 from '{}'", fd);
        }
        return false;
      }

      // check program header
      if (hdr32.e_phentsize != sizeof(Elf32_Phdr)) {
        if (!Silent) {
          LogMan::Msg::EFmt("Invalid e_phentsize32 from '{}'", fd);
        }
        return false;
      }

      // Convert to 64 bit header
      for (int i = 0; i < EI_NIDENT; i++)
        ehdr.e_ident[i] = hdr32.e_ident[i];

      #define COPY(name) ehdr.name = hdr32.name
      COPY(e_type);
      COPY(e_machine);
      COPY(e_version);
      COPY(e_entry);
      COPY(e_phoff);
      COPY(e_shoff);
      COPY(e_flags);
      COPY(e_ehsize);
      COPY(e_phentsize);
      COPY(e_phnum);
      COPY(e_shentsize);
      COPY(e_shnum);
      COPY(e_shstrndx);
      #undef COPY

      if (ehdr.e_machine != EM_386) {
        if (!Silent) {
          LogMan::Msg::EFmt("Invalid e_machine from '{}'", fd);
        }
        return false;
      }

      type = ::ELFLoader::ELFContainer::TYPE_X86_32;
    } else if (header[EI_CLASS] == ELFCLASS64) {
      if (!pread_all(fd, &ehdr, sizeof(ehdr), 0)) {
        if (!Silent) {
          LogMan::Msg::EFmt("Failed to read Ehdr64 from '{}'", fd);
        }
        return false;
      }

      // do the sizes match up as expected?

      // check elf header
      if (ehdr.e_ehsize != sizeof(ehdr)) {
        if (!Silent) {
          LogMan::Msg::EFmt("Invalid e_ehsize64 from '{}'", fd);
        }
        return false;
      }

      // check program header
      if (ehdr.e_phentsize != sizeof(Elf64_Phdr)) {
        if (!Silent) {
          LogMan::Msg::EFmt("Invalid e_phentsize64 from '{}'", fd);
        }
        return false;
      }

      if (ehdr.e_machine != EM_X86_64) {
        if (!Silent) {
          LogMan::Msg::EFmt("Invalid e_machine64 from '{}'", fd);
        }
        return false;
      }

      type = ::ELFLoader::ELFContainer::TYPE_X86_64;
    } else {
      // Unexpected elf type
      if (!Silent) {
        LogMan::Msg::EFmt("Unexpected elf type from '{}'", fd);
      }
      return false;
    }

    // sanity check program header count
    if (ehdr.e_phnum < 1 || ehdr.e_phnum > 65536 / ehdr.e_phentsize) {
      if (!Silent) {
        LogMan::Msg::EFmt("Too many program headers '{}'", fd);
      }
      return false;
    }

    if (type == ::ELFLoader::ELFContainer::TYPE_X86_32) {
      Elf32_Phdr phdrs32[ehdr.e_phnum];

      if (!pread_all(fd, phdrs32, sizeof(Elf32_Phdr) * ehdr.e_phnum, ehdr.e_phoff)) {
        if (!Silent) {
          LogMan::Msg::EFmt("Failed to read phdr32 from '{}'", fd);
        }
        return false;
      }

      // Convert to 64 bit program headers
      phdrs.resize(ehdr.e_phnum);

      for (int i = 0; i < ehdr.e_phnum; i++) {
        #define COPY(name) phdrs[i].name = phdrs32[i].name

        COPY(p_type);
        COPY(p_offset);
        COPY(p_vaddr);
        COPY(p_paddr);
        COPY(p_filesz);
        COPY(p_memsz);
        COPY(p_flags);
        COPY(p_align);

        #undef COPY
      }
    } else {
      phdrs.resize(ehdr.e_phnum);

      if (!pread_all(fd, &phdrs[0], sizeof(Elf64_Phdr) * ehdr.e_phnum, ehdr.e_phoff)) {
        if (!Silent) {
          LogMan::Msg::EFmt("Failed to read phdr64 from '{}'", fd);
        }
        return false;
      }
    }

    for (auto phdr : phdrs) {
      if (phdr.p_type == PT_INTERP) {
        InterpreterElf.resize(phdr.p_filesz);

        if (!pread_all(fd, &InterpreterElf[0], phdr.p_filesz, phdr.p_offset)) {
          if (!Silent) {
            LogMan::Msg::EFmt("Failed to read interpreter from '{}'", fd);
          }
          return false;
        }
      } else if (phdr.p_type == PT_NOTE && phdr.p_filesz < 1024) {
        uint8_t temp[phdr.p_filesz];
        if (!pread_all(fd, temp, phdr.p_filesz, phdr.p_offset)) {
          if (!Silent) {
            LogMan::Msg::EFmt("Failed to read PT_NOTE from '{}'", fd);
          }
          return false;
        }

        auto Available = phdr.p_filesz;
        auto Current = temp;
        while (Available > (12 + 4)) {
          auto note_hdr = (uint32_t *)Current;
          Available -= 12;
          Current += 12;
          if (note_hdr[2] == NT_GNU_BUILD_ID && note_hdr[0] == 4 && memcmp(&note_hdr[3], "GNU", 4) == 0) {
            Available -= 4;
            Current += 4;

            if (note_hdr[1] <= Available) {
              BuildID.resize(note_hdr[1] * 2);
              for (uint32_t i = 0; i < note_hdr[1]; i++) {
                auto val = *Current++;
                static constexpr auto toHEX = "0123456789abcdef";
                BuildID[i * 2 + 0] = toHEX[val >> 4];
                BuildID[i * 2 + 1] = toHEX[val & 15];
              }
              break;
            } else {
              break;
            }
          } else {
            auto SkipCount = FEXCore::AlignUp(note_hdr[0], 4) + FEXCore::AlignUp(note_hdr[1], 4);

            if (SkipCount <= Available) {
              Available -= SkipCount;
              Current += SkipCount;
            } else {
              break;
            }
          }
        }
      }
    }

    return true;
  }

  ptrdiff_t FileToVA(off_t FileOffset) {
    for (auto phdr : phdrs) {
      if (phdr.p_offset <= FileOffset && (phdr.p_offset + phdr.p_filesz) > FileOffset) {

        auto SectionFileOffset = FileOffset - phdr.p_offset;

        if (SectionFileOffset < phdr.p_memsz) {
          return SectionFileOffset + phdr.p_vaddr;
        }
      }
    }

    return {};
  }

  off_t VAToFile(ptrdiff_t VAOffset) {
    for (auto phdr : phdrs) {
      if (phdr.p_vaddr <= VAOffset && (phdr.p_vaddr + phdr.p_memsz) > VAOffset) {

        auto SectionVAOffset = VAOffset - phdr.p_vaddr;
        
        if (SectionVAOffset < phdr.p_filesz) {
          return SectionVAOffset + phdr.p_offset;
        }
      }
    }

    return {};
  }
  
  bool ReadElf(const std::string &file) {
    int NewFD = ::open(file.c_str(), O_RDONLY | O_NONBLOCK);
    OwnFd = true;
    return ReadElf(NewFD);
  }

  void Closefd() {
    if (OwnFd && fd != -1) {
      close(fd);
      fd = -1;
    }
  }

  ~ELFParser() {
    Closefd();
  }
};
