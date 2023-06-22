#include "CoreFileWriter/CoreFileWriter.h"
#include "Linux/Utils/ArchHelpers/UContext.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/list.h>
#include <FEXHeaderUtils/Filesystem.h>

#include <dirent.h>
#include <fmt/chrono.h>
#include <sys/wait.h>
#include <sys/procfs.h>

namespace CoreFileWriter {
  class SharedCoreFileTypes {
    public:
      struct elf_siginfo {
        int32_t si_signo;
        int32_t si_code;
        int32_t si_errno;
      };
  };

  class CoreFileWriter32Types : public SharedCoreFileTypes {
    public:
      using EHeaderType = Elf32_Ehdr;
      using PHeaderType = Elf32_Phdr;
      using NHeaderType = Elf32_Nhdr;
      using ULongType = uint32_t;
      using SizeTType = uint32_t;
      constexpr static uint32_t MachineType = EM_386;
      constexpr static uint32_t ABIVersion = 0;
      constexpr static uint32_t ELFType = ELFCLASS32;

      /**
       * @brief This doesn't match glibc mcontext_t ordering...
       */
      enum PRReg {
        // 32-bit
        REG_RBX,
        REG_RCX,
        REG_RDX,
        REG_RSI,
        REG_RDI,
        REG_RBP,
        REG_RAX,
        REG_DS,
        REG_ES,
        REG_FS,
        REG_GS,
        REG_ORIGINAL_RAX,
        REG_RIP,
        REG_CS,
        REG_EFLAGS,
        REG_RSP,
        REG_SS,
        REG_MAX,
      };

      struct timeval32 {
        int32_t tv_sec;
        int32_t tv_usec;
      };

      struct elf32_prstatus {
        elf_siginfo pr_info;
        uint16_t pr_cursig;
        uint32_t pr_sigpend;
        uint32_t pr_sighold;
        uint32_t pr_pid;
        uint32_t pr_ppid;
        uint32_t pr_pgrp;
        uint32_t pr_sid;
        struct timeval32 pr_utime;
        struct timeval32 pr_stime;
        struct timeval32 pr_cutime;
        struct timeval32 pr_cstime;
        uint32_t pr_reg[PRReg::REG_MAX];
        int pr_fpvalid;
      };

      // Architecture specific defines
      struct elf32_prpsinfo {
        int8_t pr_state;
        int8_t pr_sname;
        int8_t pr_zomb;
        int8_t pr_nice;
        uint32_t pr_flag;
        uint32_t pr_uid;
        uint32_t pr_gid;
        int32_t pr_pid, pr_ppid, pr_pgrp, pr_sid;
        char pr_fname[16];
        char pr_psargs[80];
      };

      using SiginfoType  = elf_siginfo;
      using PRStatusType = elf32_prstatus;
      using PRPSInfoType = elf32_prpsinfo;
      // FEX provides a 64-bit context even to 32-bit processes.
      using MContextSourceType = FEX::x86_64::mcontext_t;
      using FPRegSetType = FEX::x86::_libc_fpstate;
      using XStateType = FEX::x86::xstate;
      constexpr static uint32_t RSPIndex = PRReg::REG_RSP;
      constexpr static uint64_t MAXIMUM_GUEST_PAGE = (1ULL << 32);

      struct Remapping {
        size_t To;
        size_t From;
      };

      constexpr static std::array<Remapping, 11> greg_remapping = {{
        { PRReg::REG_RBX, FEX::x86_64::ContextRegs::FEX_REG_RBX },
        { PRReg::REG_RCX, FEX::x86_64::ContextRegs::FEX_REG_RCX },
        { PRReg::REG_RDX, FEX::x86_64::ContextRegs::FEX_REG_RDX },
        { PRReg::REG_RSI, FEX::x86_64::ContextRegs::FEX_REG_RSI },
        { PRReg::REG_RDI, FEX::x86_64::ContextRegs::FEX_REG_RDI },
        { PRReg::REG_RBP, FEX::x86_64::ContextRegs::FEX_REG_RBP },
        { PRReg::REG_RAX, FEX::x86_64::ContextRegs::FEX_REG_RAX },
        // TODO: Wtf is the point of this?
        { PRReg::REG_ORIGINAL_RAX, FEX::x86_64::ContextRegs::FEX_REG_RAX },
        { PRReg::REG_RIP, FEX::x86_64::ContextRegs::FEX_REG_RIP },
        { PRReg::REG_EFLAGS, FEX::x86_64::ContextRegs::FEX_REG_EFL },
        { PRReg::REG_RSP, FEX::x86_64::ContextRegs::FEX_REG_RSP },
      }};

      void SetRemainingContextValues(PRStatusType *) {
      }
  };

  class CoreFileWriter64Types : public SharedCoreFileTypes{
    public:
      using EHeaderType = Elf64_Ehdr;
      using PHeaderType = Elf64_Phdr;
      using NHeaderType = Elf64_Nhdr;
      using ULongType = uint64_t;
      using SizeTType = size_t;
      constexpr static uint32_t MachineType = EM_X86_64;
      constexpr static uint32_t ABIVersion = 0;
      constexpr static uint32_t ELFType = ELFCLASS64;

      /**
       * @brief This doesn't match glibc mcontext_t ordering...
       */
      enum PRReg {
        REG_R15 = 0,
        REG_R14,
        REG_R13,
        REG_R12,
        REG_RBP,
        REG_RBX,
        REG_R11,
        REG_R10,
        REG_R9,
        REG_R8,
        REG_RAX,
        REG_RCX,
        REG_RDX,
        REG_RSI,
        REG_RDI,
        REG_ORIGINAL_RAX,
        REG_RIP,
        REG_CS,
        REG_EFLAGS,
        REG_RSP,
        REG_SS,
        REG_FS_BASE,
        REG_GS_BASE,
        REG_DS,
        REG_ES,
        REG_FS,
        REG_GS,
        REG_MAX,
      };

      struct elf64_prstatus {
        elf_siginfo pr_info;
        uint16_t pr_cursig;
        uint64_t pr_sigpend;
        uint64_t pr_sighold;
        uint32_t pr_pid;
        uint32_t pr_ppid;
        uint32_t pr_pgrp;
        uint32_t pr_sid;
        struct timeval pr_utime;
        struct timeval pr_stime;
        struct timeval pr_cutime;
        struct timeval pr_cstime;
        uint64_t pr_reg[PRReg::REG_MAX];
        int pr_fpvalid;
      };

      // Architecture specific defines
      struct elf64_prpsinfo {
        int8_t pr_state;
        int8_t pr_sname;
        int8_t pr_zomb;
        int8_t pr_nice;
        uint64_t pr_flag;
        uint32_t pr_uid;
        uint32_t pr_gid;
        int32_t pr_pid, pr_ppid, pr_pgrp, pr_sid;
        char pr_fname[16];
        char pr_psargs[80];
      };

      using SiginfoType  = elf_siginfo;
      using PRStatusType = elf64_prstatus;
      using PRPSInfoType = elf64_prpsinfo;
      using MContextSourceType = FEX::x86_64::mcontext_t;
      using FPRegSetType = FEX::x86_64::_libc_fpstate;
      using XStateType = FEX::x86_64::xstate;
      constexpr static uint32_t RSPIndex = PRReg::REG_RSP;

      constexpr static uint64_t MAXIMUM_GUEST_PAGE = (1ULL << 48);

      struct Remapping {
        size_t To;
        size_t From;
      };
      constexpr static std::array<Remapping, 20> greg_remapping = {{
        { PRReg::REG_R15, FEX::x86_64::ContextRegs::FEX_REG_R15 },
        { PRReg::REG_R14, FEX::x86_64::ContextRegs::FEX_REG_R14 },
        { PRReg::REG_R13, FEX::x86_64::ContextRegs::FEX_REG_R13 },
        { PRReg::REG_R12, FEX::x86_64::ContextRegs::FEX_REG_R12 },
        { PRReg::REG_RBP, FEX::x86_64::ContextRegs::FEX_REG_RBP },
        { PRReg::REG_RBX, FEX::x86_64::ContextRegs::FEX_REG_RBX },
        { PRReg::REG_R11, FEX::x86_64::ContextRegs::FEX_REG_R11 },
        { PRReg::REG_R10, FEX::x86_64::ContextRegs::FEX_REG_R10 },
        { PRReg::REG_R9,  FEX::x86_64::ContextRegs::FEX_REG_R9 },
        { PRReg::REG_R8,  FEX::x86_64::ContextRegs::FEX_REG_R8 },
        { PRReg::REG_RAX, FEX::x86_64::ContextRegs::FEX_REG_RAX },
        { PRReg::REG_RCX, FEX::x86_64::ContextRegs::FEX_REG_RCX },
        { PRReg::REG_RDX, FEX::x86_64::ContextRegs::FEX_REG_RDX },
        { PRReg::REG_RSI, FEX::x86_64::ContextRegs::FEX_REG_RSI },
        { PRReg::REG_RDI, FEX::x86_64::ContextRegs::FEX_REG_RDI },
        // TODO: Wtf is the point of this?
        { PRReg::REG_ORIGINAL_RAX, FEX::x86_64::ContextRegs::FEX_REG_RAX },
        { PRReg::REG_RIP, FEX::x86_64::ContextRegs::FEX_REG_RIP },
        // TODO: Do this for real.
        { PRReg::REG_CS, FEX::x86_64::ContextRegs::FEX_REG_CSGSFS },
        { PRReg::REG_EFLAGS, FEX::x86_64::ContextRegs::FEX_REG_EFL },
        { PRReg::REG_RSP, FEX::x86_64::ContextRegs::FEX_REG_RSP },
      }};

      void SetRemainingContextValues(PRStatusType *Info) {
        Info->pr_reg[PRReg::REG_FS_BASE] = 0;
        Info->pr_reg[PRReg::REG_GS_BASE] = 0;
      }
  };

  template <class BaseTypes>
  class CoreFileWriterBase final : public CoreFileWriter, public BaseTypes {
    public:
      CoreFileWriterBase() {
        ProgramHeaders.emplace_back(ProgramHeadersToFile{.Header = &NoteHeader, .FileOffset = 0});

        Note_prstatus = EmplaceNote(CORE_NAME, NT_PRSTATUS, typename BaseTypes::PRStatusType{});
        Note_prpsinfo = EmplaceNote(CORE_NAME, NT_PRPSINFO, typename BaseTypes::PRPSInfoType{});
        Note_siginfo = EmplaceNote(CORE_NAME, NT_SIGINFO, siginfo_t{});
        struct auxv_t {};
        Note_auxv = EmplaceNote(CORE_NAME, NT_AUXV, auxv_t{});

        Note_file = EmplaceNote(CORE_NAME, NT_FILE, nt_file_t{});
        Note_fpregset = EmplaceNote(CORE_NAME, NT_FPREGSET, typename BaseTypes::FPRegSetType{});
        Note_xstate = EmplaceNote(LINUX_NAME, NT_X86_XSTATE, typename BaseTypes::XStateType{});
      }

      void ConsumeGuestAuxv(const void* auxv, uint64_t AuxvSize) override {
        Note_auxv->Data.resize(AuxvSize);
        memcpy(Note_auxv->Data.data(), auxv, AuxvSize);
      }

      void GetMappedFDs(FDFetcher Fetch) override {
        for (auto &Header : LoadSections) {
          if (Header.NTFileReference) {
            Header.FD = Fetch(Header.NTFileReference->Filename);
          }
          else {
            // Anonymous data.
          }
        }
      }

      void ConsumeMapsFD(int FD) override {
        lseek(FD, 0, SEEK_SET);
        FILE *fp = fdopen(dup(FD), "rb");
        size_t TotalDataSize = sizeof(nt_file_t) + 8;

        char Path[1024];
        char Line[1024];
        while (fgets(Line, sizeof(Path), fp) != nullptr) {
          uint64_t Begin, End, FileOffset;
          char R, W, X, P;
          if (size_t Count = sscanf(Line, "%lx-%lx %c%c%c%c %lx %*x:%*x %*x %s", &Begin, &End, &R, &W, &X, &P, &FileOffset, Path)) {
            if (Begin >= BaseTypes::MAXIMUM_GUEST_PAGE) {
              // If the address is greater than the maximum page then ignore it.
              // Ensuring large pages don't get added to 32-bit tracking.
              continue;
            }

            // TODO: VSycall should get pulled in.
            // Currently it is a JIT region that is untracked from the frontend.
            bool IsFDMap = Count == 8 && Path[0] == '/';

            FileNodeType *FileNode{};
            const fextl::string *FileString{};

            if (IsFDMap) {
              FileString = &*FileNodeStrings.emplace(Path).first;
              FileNode = &FileNodes.emplace_back(FileNodeType{
                  .FileData = {
                    .Start = static_cast<typename BaseTypes::ULongType>(Begin),
                    .End = static_cast<typename BaseTypes::ULongType>(End),
                    .File_Offset = static_cast<typename BaseTypes::ULongType>(FileOffset / 4096), // File_Offset is divided by NT_FILE header's page_size.
                  },
                  .Filename = FileString->c_str()
              });
            }

            auto &LoadPHdr = LoadSections.emplace_back(LoadSectionsToFile {
              .Header = typename BaseTypes::PHeaderType {
                .p_type = PT_LOAD,
                .p_flags =
                  (R == 'r' ? PF_R : 0U) |
                  (W == 'w' ? PF_W : 0U) |
                  (X == 'x' ? PF_X : 0U),
                .p_align = 4096,
              },
              .NTFileReference = FileNode,
              .Private = P == 'p',
            });

            LoadPHdr.Header.p_offset = 0;
            LoadPHdr.Header.p_paddr = 0;
            LoadPHdr.Header.p_vaddr = Begin;
            LoadPHdr.Header.p_filesz = 0;
            LoadPHdr.Header.p_memsz = End - Begin;

            auto &PHdr = ProgramHeaders.emplace_back(ProgramHeadersToFile{.Header = &LoadPHdr.Header, .FileOffset = 0});

            LoadPHdr.FileReference = &PHdr;

            if (IsFDMap) {
              TotalDataSize += sizeof(typename nt_file_t::Files);
              TotalDataSize += FileString->size() + 1;
            }
          }
        }
        fclose(fp);

        // Now we need to setup the NT_FILE note

        Note_file->Data.resize(TotalDataSize);
        auto NotesData = reinterpret_cast<nt_file_t*>(Note_file->Data.data());
        NotesData->page_size = 4096;
        NotesData->Count = FileNodes.size();

        {
          size_t i = 0;
          for (auto &Node : FileNodes) {
            NotesData->files[i] = Node.FileData;
            ++i;
          }
        }

        char *StartingStringLocation = reinterpret_cast<char*>(&NotesData->files[NotesData->Count]);
        size_t i = 0;
        for (auto &Node : FileNodes) {
          size_t StringLen = strlen(Node.Filename) + 1;
          memcpy(StartingStringLocation, Node.Filename, StringLen);
          StartingStringLocation += StringLen;
          ++i;
        }
      }

      void ConsumeGuestXState(FEXServerClient::CoreDump::PacketGuestXState *Req) override {
        // Consume fpstate
        auto NoteXState = reinterpret_cast<typename BaseTypes::XStateType *>(Note_xstate->Data.data());

        if (Req->AVX) {
          auto GuestXState = reinterpret_cast<const typename BaseTypes::XStateType *>(Req->OpaqueState);
          auto NoteFPState = reinterpret_cast<typename BaseTypes::FPRegSetType *>(Note_fpregset->Data.data());

          memcpy(&NoteXState->fpstate, &GuestXState->fpstate, sizeof(GuestXState->fpstate));
          auto NoteXStateHeader = reinterpret_cast<typename BaseTypes::XStateType *>(&NoteXState->fpstate.sw_reserved);
          memcpy(NoteXStateHeader, &GuestXState->xstate_hdr, sizeof(GuestXState->xstate_hdr));

          NoteXState->fpstate.ftw = 0;
          memcpy(&NoteXState->ymmh, &GuestXState->ymmh, sizeof(GuestXState->ymmh));

          memcpy(NoteFPState, &GuestXState->fpstate, sizeof(GuestXState->fpstate));
        }
        else {
          auto GuestFPState = reinterpret_cast<const typename BaseTypes::FPRegSetType *>(Req->OpaqueState);
          auto NoteFPState = reinterpret_cast<typename BaseTypes::FPRegSetType *>(Note_fpregset->Data.data());
          memcpy(NoteFPState, GuestFPState, sizeof(typename BaseTypes::FPRegSetType));

          memcpy(&NoteXState->fpstate, GuestFPState, sizeof(typename BaseTypes::FPRegSetType));
        }
      }

      void ConsumeGuestContext(FEXServerClient::CoreDump::PacketGuestContext *Req) override {
        // Consume siginfo.
        {
          auto Info = reinterpret_cast<typename BaseTypes::SiginfoType *>(Note_siginfo->Data.data());
          Info->si_signo = Req->siginfo.si_signo;
          Info->si_code = Req->siginfo.si_code;
          Info->si_errno = Req->siginfo.si_errno;
        }
        // Consume mcontext.
        {
          auto Info = reinterpret_cast<typename BaseTypes::PRStatusType *>(Note_prstatus->Data.data());

          // siginfo data is duplicated from the SIGINFO Note.
          Info->pr_info.si_signo = Req->siginfo.si_signo;
          Info->pr_info.si_code = Req->siginfo.si_code;
          Info->pr_info.si_errno = Req->siginfo.si_errno;

          Info->pr_cursig = Req->siginfo.si_signo;
          Info->pr_sigpend = Req->pr_sigpend;
          Info->pr_sighold = Req->pr_sighold;
          Info->pr_pid = PID;
          Info->pr_ppid = Req->pr_ppid;
          Info->pr_pgrp = Req->pr_pgrp;
          Info->pr_sid = Req->pr_sid;
          // Info->pr_utime = Req->pr_utime;
          // Info->pr_stime = Req->pr_stime;
          // Info->pr_cutime = Req->pr_cutime;
          // Info->pr_cstime = Req->pr_cstime;

          // pr_reg doesn't match GNU mcontext_t ordering!
          auto GuestContext = reinterpret_cast<const typename BaseTypes::MContextSourceType *>(Req->OpaqueContext);
          for (auto &Remapping : BaseTypes::greg_remapping) {
            Info->pr_reg[Remapping.To] = GuestContext->gregs[Remapping.From];
          }

          // TODO: For real.
          Info->pr_reg[BaseTypes::PRReg::REG_SS] = 0;
          Info->pr_reg[BaseTypes::PRReg::REG_DS] = 0;
          Info->pr_reg[BaseTypes::PRReg::REG_ES] = 0;
          Info->pr_reg[BaseTypes::PRReg::REG_FS] = 0;
          Info->pr_reg[BaseTypes::PRReg::REG_GS] = 0;

          BaseTypes::SetRemainingContextValues(Info);

          // Lets the coredump know that NT_FPREGSET is valid.
          Info->pr_fpvalid = 1;
        }
      }

      void Write() override {
        auto FDPair = OpenCoreDumpFile();
        if (FDPair.second == -1) {
          return;
        }

        int fd = FDPair.second;
        Header.e_phnum = ProgramHeaders.size();

        WriteHeader(fd);
        WriteProgramHeaders(fd);
        WriteNotes(fd);
        WriteLoadSections(fd);

        close(fd);
        if (CompressCoredump(FDPair.first.value())) {
          unlink(FDPair.first.value().c_str());
        }
        else {
          LogMan::Msg::IFmt("Wrote Coredump to {}", FDPair.first.value());
        }
      }

      void WriteHeader(int FD) {
        write(FD, &Header, sizeof(Header));
      }

      void WriteProgramHeaders(int FD) {
        for (auto &Header : ProgramHeaders) {
          auto CurrentOffset = GetCurrentOffset(FD);
          Header.FileOffset = CurrentOffset;
          write(FD, Header.Header, sizeof(*Header.Header));
        }
      }

      void WriteNotes(int FD) {
        auto StartingNotesFileOffset = GetCurrentOffset(FD);
        ProgramHeadersToFile *NotesHeader{};
        for (auto &Header : ProgramHeaders) {
          if (Header.Header->p_type == PT_NOTE) {
            NotesHeader = &Header;
            break;
          }
        }

        AlignFD(FD, 4);
        for (const auto &Note : Notes) {
          typename BaseTypes::NHeaderType NoteHeader{};
          NoteHeader.n_namesz = strlen(Note.Name) + 1;
          NoteHeader.n_descsz = Note.Data.size();
          NoteHeader.n_type = Note.Type;

          // Write the header
          write(FD, &NoteHeader, sizeof(NoteHeader));

          // Name follows directly afterwards
          write(FD, Note.Name, NoteHeader.n_namesz);
          AlignFD(FD, 4);

          // Data follows directly afterwards
          write(FD, Note.Data.data(), NoteHeader.n_descsz);
          AlignFD(FD, 4);
        }

        // Update notes program header to point to the correct location.
        auto EndingNotesFileOffset = GetCurrentOffset(FD);
        NotesHeader->Header->p_offset = StartingNotesFileOffset;
        NotesHeader->Header->p_filesz = EndingNotesFileOffset - StartingNotesFileOffset;
        pwrite(FD, NotesHeader->Header, sizeof(*NotesHeader->Header), NotesHeader->FileOffset);
      }

      void WriteLoadSections(int FD) {
        // Align to a page.
        AlignFD(FD, 4096);

        for (auto &Header : LoadSections) {
          bool ShouldWrite = true;
          if (Header.FD != -1) {
            if (Header.Private &&
                (CoreDumpFilter & CoreDumpFilterBits::DUMP_FILE_PRIVATE) == 0) {
              // If the section is private, and the Coredump filter isn't active for private sections, skip it.
              ShouldWrite = false;
            }
            if (!Header.Private &&
                (CoreDumpFilter & CoreDumpFilterBits::DUMP_FILE_SHARED) == 0) {
              // If the section is shared, and the Coredump filter isn't active for shared sections, skip it.
              ShouldWrite = false;
            }
          }
          else {
            auto Info = reinterpret_cast<typename BaseTypes::PRStatusType*>(Note_prstatus->Data.data());
            auto RSP = Info->pr_reg[BaseTypes::RSPIndex];
            // If this header intersects with the stack pointer, then always recover it.
            if (!(RSP >= Header.Header.p_vaddr && RSP < (Header.Header.p_vaddr + Header.Header.p_memsz))) {
              if (Header.Private &&
                  (CoreDumpFilter & CoreDumpFilterBits::DUMP_ANONYMOUS_PRIVATE) == 0) {
                // If the section is private, and the Coredump filter isn't active for private sections, skip it.
                ShouldWrite = false;
              }
              if (!Header.Private &&
                  (CoreDumpFilter & CoreDumpFilterBits::DUMP_ANONYMOUS_SHARED) == 0) {
                // If the section is shared, and the Coredump filter isn't active for shared sections, skip it.
                ShouldWrite = false;
              }
            }
          }

          auto CurrentOffset = GetCurrentOffset(FD);
          Header.Header.p_offset = CurrentOffset;

          if (ShouldWrite) {
            if (Header.FD != -1) {
              Header.Header.p_filesz = Header.NTFileReference->FileData.End - Header.NTFileReference->FileData.Start;
              off64_t OffsetIn = Header.NTFileReference->FileData.File_Offset * 4096;
              off64_t OffsetOut = CurrentOffset;
              WriteZero(FD, Header.Header.p_filesz);

              // Sometimes cross process copy_file_range doesn't work.
              // Can return -EXDEV even if on the same filesystem. (Tested on kernel 6.3.3).
              if (copy_file_range(Header.FD, &OffsetIn, FD, &OffsetOut, Header.Header.p_filesz, 0) == -1) {
                OffsetIn = Header.NTFileReference->FileData.File_Offset * 4096;
                // In the case of failure to copy_file_range then try to use sendfile.
                // Seek to the Previous current offset before we increased the size.
                lseek(FD, CurrentOffset, SEEK_SET);
                if (sendfile(FD, Header.FD, &OffsetIn, Header.Header.p_filesz) == -1) {
                  // If sendfile also failed then just seek to the end, so it fills with zeros.
                  lseek(FD, 0, SEEK_END);
                }

                // Sendfile updates the output file's offset. No need to SEEK_END if it succeeded.
              }
            }
            else {
              // Read the mapping from the guest.
              fextl::vector<char> Data(Header.Header.p_memsz);
              Header.Header.p_filesz = Header.Header.p_memsz;

              if (MemoryFetch(Data.data(), Header.Header.p_vaddr, Header.Header.p_memsz)) {
                write(FD, Data.data(), Data.size());
              }
              else {
                WriteZero(FD, Header.Header.p_filesz);
              }
            }
          }

          // Update load section with offset
          pwrite(FD, &Header.Header, sizeof(Header.Header), Header.FileReference->FileOffset);
        }
      }

      ~CoreFileWriterBase() override {
        for (auto &Node : LoadSections) {
          if (Node.FD != -1) {
            close(Node.FD);
            Node.FD = -1;
          }
        }
      }

    protected:
      typename BaseTypes::EHeaderType Header {
        .e_ident = {
          ELFMAG0,    ELFMAG1,     ELFMAG2,    ELFMAG3,
          BaseTypes::ELFType, ELFDATA2LSB, EV_CURRENT, ELFOSABI_NONE,
          BaseTypes::ABIVersion, // ABI Version
          0, 0, 0, 0, 0, 0, 0, // Padding
        },
        .e_type = ET_CORE,
        .e_machine = BaseTypes::MachineType,
        .e_version = EV_CURRENT,
        .e_entry = 0,
        .e_phoff = sizeof(typename BaseTypes::EHeaderType), // Program headers immediately follow the header.
        .e_shoff = 0,
        .e_flags = 0,
        .e_ehsize = sizeof(typename BaseTypes::EHeaderType),
        .e_phentsize = sizeof(typename BaseTypes::PHeaderType),
        .e_phnum = 0,
        .e_shentsize = 0,
        .e_shnum = 0,
        .e_shstrndx = 0,
      };

      typename BaseTypes::PHeaderType NoteHeader {
        .p_type = PT_NOTE,
      };

      struct ProgramHeadersToFile {
        typename BaseTypes::PHeaderType *Header;
        typename BaseTypes::SizeTType FileOffset{};
      };

      struct nt_file_t {
        typename BaseTypes::ULongType Count{}; ///< Number of mapped files.
        typename BaseTypes::ULongType page_size {4096}; ///< Unit scale for File_Offset
        struct Files {
          typename BaseTypes::ULongType Start;
          typename BaseTypes::ULongType End;
          typename BaseTypes::ULongType File_Offset;
        };

        Files files[];
      };

      struct FileNodeType {
        typename nt_file_t::Files FileData;
        const char *Filename;
      };

      struct LoadSectionsToFile {
        typename BaseTypes::PHeaderType Header;
        ProgramHeadersToFile *FileReference;
        FileNodeType *NTFileReference;
        bool Private{};

        // FD Maps
        int FD{-1};

        // Memory maps
      };

      // Note names.
      constexpr static char CORE_NAME[] = "CORE";
      constexpr static char LINUX_NAME[] = "LINUX";

      struct NoteEntry {
        const char *Name;
        uint32_t Type;
        std::vector<char> Data;
      };

      fextl::list<FileNodeType> FileNodes;
      fextl::list<LoadSectionsToFile> LoadSections;
      fextl::list<ProgramHeadersToFile> ProgramHeaders{};
      fextl::list<NoteEntry> Notes;
      fextl::unordered_set<fextl::string> FileNodeStrings;

      template<typename T>
      NoteEntry *EmplaceNote(const char *Name, uint32_t Type, T NoteData) {
        auto &Note = Notes.emplace_back(NoteEntry{
          .Name = Name,
          .Type = Type,
        });

        Note.Data.resize(sizeof(T));
        T* NoteDataBacking = reinterpret_cast<T*>(Note.Data.data());
        *NoteDataBacking = NoteData;
        return &Note;
      }

      NoteEntry *Note_prstatus{};
      NoteEntry *Note_prpsinfo{};
      NoteEntry *Note_siginfo{};
      NoteEntry *Note_file{};
      NoteEntry *Note_fpregset{};
      NoteEntry *Note_xstate{};
      NoteEntry *Note_auxv{};
  };

  std::pair<std::optional<fextl::string>, int> CoreFileWriter::OpenCoreDumpFile() {
    if ((CoreDumpFilter & CoreDumpFilterBits::DUMP_STYLE_MASK) == CoreDumpFilterBits::DUMP_DISABLE) {
      // Return early if dumping is disabled.
      return {std::nullopt, -1};
    }

    fextl::string CoreDumpFolder{};
    if (CoreDumpConfigFolder().empty()) {
      CoreDumpFolder = fextl::fmt::format("{}/coredump", FEXCore::Config::GetDataDirectory());
    }
    else {
      CoreDumpFolder = CoreDumpConfigFolder();
    }

    if (FHU::Filesystem::CreateDirectories(CoreDumpFolder)) {
      const auto CoreDumpFile = fextl::fmt::format("{}/core.{}.{}.{}.{:%a %Y-%m-%d %H:%M:%S %Z}.coredump",
        CoreDumpFolder,
        ApplicationName,
        UID,
        PID,
        fmt::localtime(Timestamp)
      );

      int FD = open(CoreDumpFile.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
      return {CoreDumpFile, FD};
    }

    return {std::nullopt, -1};
  }

  bool CoreFileWriter::CompressCoredump(const std::string_view File) {
    if (!CompressZSTD()) {
      // If not enabled then don't even try compressing.
      return false;
    }

    const auto CompressedCoredump = fextl::fmt::format("{}.zstd", File);
    const auto CompressCommand = fextl::fmt::format("zstd -q -q --no-progress \"{}\" -o \"{}\"", File, CompressedCoredump);

    // Don't ignore SIGCHLD here. We inherited this from our parent.
    signal(SIGCHLD, SIG_DFL);

    int pid = fork();
    if (pid == 0) {
      // Child
      const char *argv[8];
      argv[0] = "zstd";
      argv[1] = "-q";
      argv[2] = "-q";
      argv[3] = "--no-progress";
      argv[4] = File.data();
      argv[5] = "-o";
      argv[6] = CompressedCoredump.c_str();
      argv[7] = nullptr;

      // Try and execute {erofsfuse, squashfuse} to mount our rootfs
      if (execvpe(argv[0], (char * const*)argv, environ) == -1) {
        // End the child
        exit(1);
      }
    }
    else {
      // Parent
      // This will happen with execvpe of squashmount or exit on failure
      int WaitStatus{};
      int Ret = waitpid(pid, &WaitStatus, 0);

      if (Ret != -1 && WIFEXITED(WaitStatus) && WEXITSTATUS(WaitStatus) == 0) {
        LogMan::Msg::IFmt("Wrote Coredump to {}", CompressedCoredump);
        return true;
      }

    }
    return false;
  }

  void CoreFileWriter::CleanupOldCoredumps() {
    const auto DeletionAge = std::chrono::seconds(CoreDumpMaximumAge());

    auto CurrentTime = std::chrono::system_clock::now();

    fextl::string CoreDumpFolder{};
    if (CoreDumpConfigFolder().empty()) {
      CoreDumpFolder = fextl::fmt::format("{}/coredump", FEXCore::Config::GetDataDirectory());
    }
    else {
      CoreDumpFolder = CoreDumpConfigFolder();
    }
    auto FD = open(CoreDumpFolder.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY);
    DIR *dir = fdopendir(dup(FD));

    struct dirent *entry;
    uint64_t TotalCoreDumpSize{};

    struct CoreDumps {
      time_t Time;
      off_t Size;
      fextl::string Name;
    };
    auto SortByTime = [](CoreDumps &lhs, CoreDumps &rhs) {
      return lhs.Time < rhs.Time;
    };

    fextl::vector<CoreDumps> DumpsRemaining;
    while ((entry = readdir(dir)) != nullptr) {
      struct stat buf{};
      if (fstatat(FD, entry->d_name, &buf, 0) == 0) {
        if (!S_ISREG(buf.st_mode)) {
          // Skip non-regular files.
          continue;
        }

        auto Diff = CurrentTime - std::chrono::system_clock::from_time_t(buf.st_ctime);
        if (Diff >= DeletionAge) {
          // If the creation time of this is more than a day old, just erase it.
          unlinkat(FD, entry->d_name, 0);
        }
        else {
          DumpsRemaining.emplace_back(CoreDumps {
            .Time = buf.st_ctime,
            .Size = buf.st_size,
            .Name = entry->d_name,
          });
          TotalCoreDumpSize += buf.st_size;
        }
      }
    }
    closedir(dir);

    // Ensure that the core dumps don't get too large.
    // Delete from the oldest to the newest until we get under the maximum coredump size.
    if (TotalCoreDumpSize >= CoreDumpMaximumSize()) {
      std::sort(DumpsRemaining.begin(), DumpsRemaining.end(), SortByTime);

      auto iter = DumpsRemaining.begin();
      while (TotalCoreDumpSize >= CoreDumpMaximumSize() && iter != DumpsRemaining.end()) {
        unlinkat(FD, iter->Name.c_str(), 0);
        TotalCoreDumpSize -= iter->Size;
        iter = DumpsRemaining.erase(iter);
      }
    }

    close(FD);
  }

  using CoreFileWriter64 = CoreFileWriterBase<CoreFileWriter64Types>;
  using CoreFileWriter32 = CoreFileWriterBase<CoreFileWriter32Types>;

  fextl::unique_ptr<CoreFileWriter> CreateWriter64() {
    return fextl::make_unique<CoreFileWriter64>();
  }

  fextl::unique_ptr<CoreFileWriter> CreateWriter32() {
    return fextl::make_unique<CoreFileWriter32>();
  }
}
