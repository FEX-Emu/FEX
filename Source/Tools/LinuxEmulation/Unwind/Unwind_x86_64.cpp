#include "LinuxSyscalls/SignalDelegator.h"
#include "LinuxSyscalls/Syscalls.h"
#include "Unwind/FileMapping.h"
#include "Unwind/ELFMapping.h"

#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/ArchHelpers/Arm64.h>
#include <FEXCore/Utils/FPState.h>
#include <FEXCore/Utils/MathUtils.h>

#include "Unwind_x86_64.h"
#include <dirent.h>
#include <signal.h>
#include <filesystem>
#include <libunwind-x86_64.h>

namespace ELFMapping {
  struct ELFMemMapping;
}

namespace Unwind::x86_64 {
#define panic(args...) \
  { LogMan::Msg::AFmt(args); }
#define FORWARD_DECLARE
#include "Unwind/Unwind_accessors.inl"

  class Unwinder_x86_64 final : public Unwinder {
  public:
    Unwinder_x86_64(void *Info, void *Context) {
      siginfo = *(siginfo_t*)Info;
      _context = *(FEXCore::x86_64::ucontext_t*)Context;

      addr = unw_create_addr_space(&accessors, __LITTLE_ENDIAN);

      if (unw_init_remote (&cursor, addr, this) < 0) {
        panic ("unw_init_remote failed!");
        CanBacktrace = false;
      }

      LoadFileMappings();
    }
    void Backtrace() override;

    FileMapping::FileMapping* GetFileMapping(uint64_t Addr);

    std::optional<uint64_t> TryPeekMem(uint64_t Addr, uint32_t Size) {
      uint64_t Result {};
      if (FEX::HLE::FaultSafeUserMemAccess::CopyFromUser(&Result, (void*)Addr, Size) == 0) {
        return Result;
      }

      return std::nullopt;
    }

    unw_word_t GetReg(unw_regnum_t regnum) {
      switch (regnum) {
#define GETREG(x) case UNW_X86_64_##x: return _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_##x]; break
        GETREG(RAX);
        GETREG(RDX);
        GETREG(RCX);
        GETREG(RBX);
        GETREG(RSI);
        GETREG(RDI);
        GETREG(RBP);
        GETREG(RSP);
        GETREG(R8);
        GETREG(R9);
        GETREG(R10);
        GETREG(R11);
        GETREG(R12);
        GETREG(R13);
        GETREG(R14);
        GETREG(R15);
        GETREG(RIP);
#undef GETREG
        default:
          panic("Unhandled access regnum: %d", regnum);
      }
      return 0;
    }
    void SetReg(unw_regnum_t regnum, unw_word_t Value) {
      switch (regnum) {
#define GETREG(x) case UNW_X86_64_##x: _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_##x] = Value; break
        GETREG(RAX);
        GETREG(RDX);
        GETREG(RCX);
        GETREG(RBX);
        GETREG(RSI);
        GETREG(RDI);
        GETREG(RBP);
        GETREG(RSP);
        GETREG(R8);
        GETREG(R9);
        GETREG(R10);
        GETREG(R11);
        GETREG(R12);
        GETREG(R13);
        GETREG(R14);
        GETREG(R15);
        GETREG(RIP);
#undef GETREG
        default:
          panic("Unhandled access regnum: %d", regnum);
      }
    }

    void BacktraceHeader() {
      LogMan::Msg::EFmt("Signal: {}", siginfo.si_signo);
      LogMan::Msg::EFmt("Register data");
      LogMan::Msg::EFmt("RIP: {:016x}", _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RIP]);
      LogMan::Msg::EFmt("RAX: {:016x} RBX: {:016x} RCX: {:016x} RDX {:016x}",
        _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RAX], _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RBX],
        _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RCX], _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RDX]);
      LogMan::Msg::EFmt("RSI: {:016x} RDI: {:016x} RSP: {:016x} RBP {:016x}",
        _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RSI], _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RDI],
        _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RSP], _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RBP]);
      LogMan::Msg::EFmt("R8:  {:016x} R9:  {:016x} R10: {:016x} R11 {:016x}",
        _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R8], _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R9],
        _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R10], _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R11]);
      LogMan::Msg::EFmt("R12: {:016x} R13: {:016x} R14: {:016x} R15 {:016x}",
        _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R12], _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R13],
        _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R14], _context.uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R15]);
    }

    public:
    std::list<FileMapping::FileMapping> FileMappings;
    std::map<std::string, FileMapping::FileMapping*> PathToFileMap;
    void LoadFileMappings();

    bool CanBacktrace {};
    siginfo_t siginfo {};
    FEXCore::x86_64::ucontext_t _context {};
    unw_cursor_t cursor {};
    unw_addr_space_t addr {};

    unw_accessors_t accessors {
      .find_proc_info         = UnwinderFuncs::find_proc_info,
        .put_unwind_info        = UnwinderFuncs::put_unwind_info,
        .get_dyn_info_list_addr = UnwinderFuncs::get_dyn_info_list_addr,
        .access_mem             = UnwinderFuncs::access_mem,
        .access_reg             = UnwinderFuncs::access_reg,
        .access_fpreg           = UnwinderFuncs::access_fpreg,
        .resume                 = UnwinderFuncs::resume,
        .get_proc_name          = UnwinderFuncs::get_proc_name,
    };
  };

void Unwinder_x86_64::LoadFileMappings() {
  int FD = open("/proc/self/map_files", O_RDONLY | O_CLOEXEC | O_DIRECTORY);

  if (FD == -1) {
    return;
  }

  lseek(FD, 0, SEEK_SET);
  DIR *dir = fdopendir(dup(FD));
  struct dirent *entry;
  char Tmp[512];
  FileMapping::FileMapping *CurrentMap{};
  while ((entry = readdir(dir)) != nullptr) {
    struct stat buf{};
    if (fstatat(FD, entry->d_name, &buf, 0)) {
      int LinkSize = readlinkat(FD, entry->d_name, Tmp, 512);
      std::string_view ReadlinkView;
      if (LinkSize > 0) {
        ReadlinkView = std::string_view(Tmp, LinkSize);
        uint64_t Begin, End;
        if (sscanf(entry->d_name, "%lx-%lx", &Begin, &End) == 2) {
          if (CurrentMap && CurrentMap->Path == ReadlinkView) {
            // Extending file mapping
            CurrentMap->End = End;
          }
          else {
            // Start a new file mapping
            CurrentMap = &FileMappings.emplace_back(FileMapping::FileMapping{Begin, End, std::string(ReadlinkView)});
            PathToFileMap.emplace(CurrentMap->Path, CurrentMap);
          }
        }
      }
      else {
        ReadlinkView = "";
      }
    }
  }
  closedir(dir);
}

void Unwinder_x86_64::Backtrace() {
  BacktraceHeader();

  int ret;
  int Frame {};
  char name[256];

  do
  {
    unw_word_t ip, sp, off;
    unw_get_reg (&cursor, UNW_REG_IP, &ip);
    unw_get_reg (&cursor, UNW_REG_SP, &sp);
    bool NoProc = false;
    std::string Buffer{};
    if (unw_get_proc_name (&cursor, name, sizeof (name), &off) == 0)
    {
      if (off)
        Buffer = fmt::format("<{} + 0x{:x}>", name, (long) off);
      else
        Buffer = fmt::format("<{}>", name);
    }
    else {
      NoProc = true;
    }

    LogMan::Msg::EFmt(
      sizeof(unw_word_t) == 4 ?
      "                #{}  {}0x{:08x} {:<32} sp=(0x{:08x})" :
      "                #{}  {}0x{:016x} {:<32} sp=(0x{:016x})"
      , Frame, NoProc ? "NoELFParse: " : "", (long) ip, Buffer, (long) sp);

    ret = unw_step (&cursor);

    ++Frame;
    if (ret <= 0)
    {
      auto prev_rip = ip;
      unw_get_reg (&cursor, UNW_REG_IP, &ip);
      LogMan::Msg::EFmt("FAILURE: unw_step() returned {} for ip: 0x{:x} prev_ip: 0x{:x}", ret, (long) ip, (uint64_t)prev_rip);
      return;
    }
  }
  while (ret > 0);
}

FileMapping::FileMapping* Unwinder_x86_64::GetFileMapping(uint64_t Addr) {
  for (auto& Mapping : FileMappings) {
    if (Mapping.Begin <= Addr && Mapping.End > Addr) {
      return &Mapping;
    }
  }
  return nullptr;
}

#define ACCESSOR_IMPL
#define UNWINDER_TYPE Unwinder_x86_64
#include "Unwind/Unwind_accessors.inl"

  Unwinder *Unwind(void *Info, void *Context) {
    return new Unwinder_x86_64(Info, Context);
  }
}
