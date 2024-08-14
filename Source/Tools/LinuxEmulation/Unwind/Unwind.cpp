// SPDX-License-Identifier: MIT
#include "ArchHelpers/UContext.h"
#include "Unwind/Unwind.h"
#include "LinuxSyscalls/ThreadManager.h"

#include <FEXHeaderUtils/Filesystem.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/unordered_map.h>
#include <cstring>
#include <dirent.h>

#ifdef UNWIND_X86_64
#include <libunwind-x86_64.h>
#define NAMESPACE Unwind::x86_64
#else
#include <libunwind-x86.h>
#define NAMESPACE Unwind::x86
#endif
namespace NAMESPACE {
int find_proc_info(unw_addr_space_t as, unw_word_t ip, unw_proc_info_t* pip, int need_unwind_info, void* arg);
void put_unwind_info(unw_addr_space_t as, unw_proc_info_t* pip, void* arg);
int get_dyn_info_list_addr(unw_addr_space_t as, unw_word_t* dilap, void* arg);
int access_mem(unw_addr_space_t as, unw_word_t addr, unw_word_t* valp, int write, void* arg);
int access_reg(unw_addr_space_t as, unw_regnum_t regnum, unw_word_t* valp, int write, void* arg);
int access_fpreg(unw_addr_space_t as, unw_regnum_t regnum, unw_fpreg_t* fpvalp, int write, void* arg);
int resume(unw_addr_space_t as, unw_cursor_t* cp, void* arg);
int get_proc_name(unw_addr_space_t as, unw_word_t addr, char* bufp, size_t buf_len, unw_word_t* offp, void* arg);

struct MemMapping {
  uint64_t Begin, End;
  uint32_t Perm;
};

struct FileMapping {
  uint64_t Begin, End;
  fextl::string Path;
  fextl::vector<MemMapping*> MemMappings;
};

class Unwinder final : public Unwind::Unwinder {
private:
#ifdef UNWIND_X86_64
  using ContextType = FEXCore::x86_64::ucontext_t;
  using SigInfoType = siginfo_t;
#else
  using ContextType = FEXCore::x86::ucontext_t;
  using SigInfoType = FEXCore::x86::siginfo_t;
#endif
public:
  Unwinder() {
    addr = unw_create_addr_space(&accessors, __LITTLE_ENDIAN);
  }

  void Backtrace(FEX::HLE::ThreadStateObject* Thread, void* Info, void* Context) override;

  ~Unwinder() {}

  unw_word_t GetReg(unw_regnum_t regnum);

  bool AddressHasAccess(uint64_t Addr, bool Write);
  FileMapping* GetFileMapping(uint64_t Addr);
private:
  void BacktraceHeader(ContextType* Context);
  FEX::HLE::ThreadStateObject* Thread;
  SigInfoType* Info;
  ContextType* Context;

  unw_cursor_t cursor;
  unw_addr_space_t addr;

  unw_accessors_t accessors {
    .find_proc_info = find_proc_info,
    .put_unwind_info = put_unwind_info,
    .get_dyn_info_list_addr = get_dyn_info_list_addr,
    .access_mem = access_mem,
    .access_reg = access_reg,
    .access_fpreg = access_fpreg,
    .resume = resume,
    .get_proc_name = get_proc_name,
  };

  fextl::vector<MemMapping> MemMappings;
  fextl::list<FileMapping> FileMappings;
  fextl::unordered_map<fextl::string, FileMapping*> PathToFileMap;
  void LoadMappedFiles();
  void LoadMemMappings();
  FileMapping* FindFileMapping(const fextl::string& Path);

#ifdef UNWIND_X86_64
  constexpr static auto FEX_RIP = FEXCore::x86_64::FEX_REG_RIP;
  constexpr static auto FEX_RAX = FEXCore::x86_64::FEX_REG_RAX;
  constexpr static auto FEX_RBX = FEXCore::x86_64::FEX_REG_RBX;
  constexpr static auto FEX_RCX = FEXCore::x86_64::FEX_REG_RCX;
  constexpr static auto FEX_RDX = FEXCore::x86_64::FEX_REG_RDX;
  constexpr static auto FEX_RSI = FEXCore::x86_64::FEX_REG_RSI;
  constexpr static auto FEX_RDI = FEXCore::x86_64::FEX_REG_RDI;
  constexpr static auto FEX_RSP = FEXCore::x86_64::FEX_REG_RSP;
  constexpr static auto FEX_RBP = FEXCore::x86_64::FEX_REG_RBP;
#else
  constexpr static auto FEX_RIP = FEXCore::x86::FEX_REG_EIP;
  constexpr static auto FEX_RAX = FEXCore::x86::FEX_REG_RAX;
  constexpr static auto FEX_RBX = FEXCore::x86::FEX_REG_RBX;
  constexpr static auto FEX_RCX = FEXCore::x86::FEX_REG_RCX;
  constexpr static auto FEX_RDX = FEXCore::x86::FEX_REG_RDX;
  constexpr static auto FEX_RSI = FEXCore::x86::FEX_REG_RSI;
  constexpr static auto FEX_RDI = FEXCore::x86::FEX_REG_RDI;
  constexpr static auto FEX_RSP = FEXCore::x86::FEX_REG_RSP;
  constexpr static auto FEX_RBP = FEXCore::x86::FEX_REG_RBP;
#endif
};

FileMapping* Unwinder::FindFileMapping(const fextl::string& Path) {
  auto it = PathToFileMap.find(Path);
  if (it == PathToFileMap.end()) {
    return nullptr;
  }

  return it->second;
}

void Unwinder::LoadMappedFiles() {
  int FD = open("/proc/self/map_files", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  DIR* dir = fdopendir(dup(FD));
  if (dir) {
    struct dirent* entry {};
    struct stat buf {};
    char Tmp[PATH_MAX];
    FileMapping* CurrentMap {};
    while ((entry = readdir(dir)) != nullptr) {
      ///< != 0 skips `.` and `..`
      if (fstatat(FD, entry->d_name, &buf, 0) != 0) {
        int LinkSize = readlinkat(FD, entry->d_name, Tmp, sizeof(Tmp));
        if (LinkSize > 0) {
          auto ReadlinkView = std::string_view(Tmp, LinkSize);
          uint64_t Begin, End;
          if (sscanf(entry->d_name, "%lx-%lx", &Begin, &End) == 2) {
            if (CurrentMap && CurrentMap->Path == ReadlinkView) {
              // Extending file mapping
              CurrentMap->End = End;
            } else {
              // Start a new file mapping
              CurrentMap = &FileMappings.emplace_back(FileMapping {Begin, End, fextl::string(ReadlinkView)});
              PathToFileMap.emplace(CurrentMap->Path, CurrentMap);
            }
          }
        }
      }
    }
    closedir(dir);
  }
  close(FD);
}

void Unwinder::LoadMemMappings() {
  ///< TODO: This should use `FaultSafeUserMemAccess:CheckIf{Readable,Writable}`
  FILE* fp = fopen("/proc/self/maps", "rb");
  char Line[1024];
  if (fp) {
    while (fgets(Line, sizeof(Line), fp) != nullptr) {
      uint64_t Begin, End;
      char R, W, X, P;
      if (size_t Read = sscanf(Line, "%lx-%lx %c%c%c%c %*lx %*x:%*x %*x %s", &Begin, &End, &R, &W, &X, &P, Line)) {
        auto FileMapping = FindFileMapping(Line);

        auto Mapping = &MemMappings.emplace_back(MemMapping {
          .Begin = Begin,
          .End = End,
          .Perm = (R == 'r' ? (1U << 3) : 0) | (W == 'w' ? (1U << 2) : 0) | (X == 'x' ? (1U << 1) : 0) | (P == 'p' ? (1U << 0) : 0),
        });

        if (FileMapping) {
          FileMapping->MemMappings.emplace_back(Mapping);
        }
      }
    }
    fclose(fp);
  }
}

FileMapping* Unwinder::GetFileMapping(uint64_t Addr) {
  for (auto& Mapping : FileMappings) {
    if (Mapping.Begin <= Addr && Mapping.End > Addr) {
      return &Mapping;
    }
  }
  return nullptr;
}

bool Unwinder::AddressHasAccess(uint64_t Addr, bool Write) {
  for (const auto& Mapping : MemMappings) {
    if (Mapping.Begin <= Addr && Mapping.End > Addr) {
      if (Write) {
        return Mapping.Perm & (1U << 2);
      } else {
        return Mapping.Perm & (1U << 3);
      }
    }
  }

  return false;
}

void Unwinder::BacktraceHeader(ContextType* Context) {
  fextl::fmt::print(stderr, "RIP: {:016x}\n", Context->uc_mcontext.gregs[FEX_RIP]);
  fextl::fmt::print(stderr, "RAX: {:016x} RBX: {:016x} RCX: {:016x} RDX {:016x}\n", Context->uc_mcontext.gregs[FEX_RAX],
                    Context->uc_mcontext.gregs[FEX_RBX], Context->uc_mcontext.gregs[FEX_RCX], Context->uc_mcontext.gregs[FEX_RDX]);
  fextl::fmt::print(stderr, "RSI: {:016x} RDI: {:016x} RSP: {:016x} RBP {:016x}\n", Context->uc_mcontext.gregs[FEX_RSI],
                    Context->uc_mcontext.gregs[FEX_RDI], Context->uc_mcontext.gregs[FEX_RSP], Context->uc_mcontext.gregs[FEX_RBP]);

#ifdef UNWIND_X86_64
  fextl::fmt::print(stderr, "R8:  {:016x} R9:  {:016x} R10: {:016x} R11 {:016x}\n", Context->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R8],
                    Context->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R9], Context->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R10],
                    Context->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R11]);
  fextl::fmt::print(stderr, "R12: {:016x} R13: {:016x} R14: {:016x} R15 {:016x}\n",
                    Context->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R12], Context->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R13],
                    Context->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R14], Context->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_R15]);
#endif

  fextl::fmt::print(stderr, "\n");
}

unw_word_t Unwinder::GetReg(unw_regnum_t regnum) {
#ifdef UNWIND_X86_64
  constexpr auto UNW_REG_RIP = UNW_X86_64_RIP;
  constexpr auto UNW_REG_RAX = UNW_X86_64_RAX;
  constexpr auto UNW_REG_RBX = UNW_X86_64_RBX;
  constexpr auto UNW_REG_RCX = UNW_X86_64_RCX;
  constexpr auto UNW_REG_RDX = UNW_X86_64_RDX;
  constexpr auto UNW_REG_RSI = UNW_X86_64_RSI;
  constexpr auto UNW_REG_RDI = UNW_X86_64_RDI;
  constexpr auto UNW_REG_RBP = UNW_X86_64_RBP;
  constexpr auto UNW_REG_RSP = UNW_X86_64_RSP;
#else
  constexpr auto UNW_REG_RIP = UNW_X86_EIP;
  constexpr auto UNW_REG_RAX = UNW_X86_EAX;
  constexpr auto UNW_REG_RBX = UNW_X86_EBX;
  constexpr auto UNW_REG_RCX = UNW_X86_ECX;
  constexpr auto UNW_REG_RDX = UNW_X86_EDX;
  constexpr auto UNW_REG_RSI = UNW_X86_ESI;
  constexpr auto UNW_REG_RDI = UNW_X86_EDI;
  constexpr auto UNW_REG_RBP = UNW_X86_EBP;
  constexpr auto UNW_REG_RSP = UNW_X86_ESP;
#endif
  switch (regnum) {
#define GETREG(y) \
  case UNW_REG_##y: return Context->uc_mcontext.gregs[FEX_##y]; break
    GETREG(RAX);
    GETREG(RDX);
    GETREG(RCX);
    GETREG(RBX);
    GETREG(RSI);
    GETREG(RDI);
    GETREG(RBP);
    GETREG(RSP);
    GETREG(RIP);
#undef GETREG
#ifdef UNWIND_X86_64
#define GETREG(y) \
  case UNW_X86_64_##y: return Context->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_##y]; break
    GETREG(R8);
    GETREG(R9);
    GETREG(R10);
    GETREG(R11);
    GETREG(R12);
    GETREG(R13);
    GETREG(R14);
    GETREG(R15);
#undef GETREG
#endif
  default: fmt::print(stderr, "Unhandled access regnum: %d", regnum);
  }
  return 0;
}

void Unwinder::Backtrace(FEX::HLE::ThreadStateObject* _Thread, void* _Info, void* _Context) {
  Thread = _Thread;
  Info = reinterpret_cast<SigInfoType*>(_Info);
  Context = reinterpret_cast<ContextType*>(_Context);

  LoadMappedFiles();
  LoadMemMappings();

  if (unw_init_remote(&cursor, addr, this) < 0) {
    LogMan::Msg::EFmt("unw_init_remote failed!\n");
    return;
  }

  BacktraceHeader(Context);

  int ret;
  int Frame {};
  char name[256];

  fextl::fmt::print(stderr, "                Stack trace of thread {}:\n", Thread->ThreadInfo.TID.load());

  do {
    unw_word_t ip, sp, off;
    unw_get_reg(&cursor, UNW_REG_IP, &ip);
    unw_get_reg(&cursor, UNW_REG_SP, &sp);
    bool NoProc = false;
    fextl::string Buffer {};
    if (unw_get_proc_name(&cursor, name, sizeof(name), &off) == 0) {
      if (off) {
        Buffer = fextl::fmt::format("<{} + 0x{:x}>", name, (long)off);
      } else {
        Buffer = fextl::fmt::format("<{}>", name);
      }
    } else {
      NoProc = true;
    }

    fextl::fmt::print(stderr,
                      sizeof(unw_word_t) == 4 ? "                #{}  {}0x{:08x} {:<32} sp=(0x{:08x})\n" :
                                                "                #{}  {}0x{:016x} {:<32} sp=(0x{:016x})\n",
                      Frame, NoProc ? "NoELFParse: " : "", (long)ip, Buffer, (long)sp);

    ret = unw_step(&cursor);

    ++Frame;
    if (ret < 0) {
      unw_get_reg(&cursor, UNW_REG_IP, &ip);
      fextl::fmt::print(stderr, "FAILURE: unw_step() returned {} for ip: 0x{:x}\n", ret, (long)ip);
      return;
    }
  } while (ret > 0);
}

fextl::unique_ptr<Unwind::Unwinder> Unwind() {
  return fextl::make_unique<Unwinder>();
}

int find_proc_info(unw_addr_space_t as, unw_word_t ip, unw_proc_info_t* pip, int need_unwind_info, void* arg) {
  // Just claim we don't have anything.
  return -UNW_ENOINFO;
}

void put_unwind_info(unw_addr_space_t as, unw_proc_info_t* pip, void* arg) {
  // Unused.
}

int get_dyn_info_list_addr(unw_addr_space_t as, unw_word_t* dilap, void* arg) {
  // Say we don't have one.
  *dilap = 0;
  return 0;
}

int access_mem(unw_addr_space_t as, unw_word_t addr, unw_word_t* valp, int write, void* arg) {
  auto Unwind = (NAMESPACE::Unwinder*)arg;
  if (!Unwind->AddressHasAccess(addr, write != 0)) {
    // If the guest doesn't have access, then don't try reading.
    return -UNW_EUNSPEC;
  }

  if (write == 0) {
    memcpy(valp, reinterpret_cast<void*>(addr), sizeof(unw_word_t));
  }
  return 0;
}
int access_reg(unw_addr_space_t as, unw_regnum_t regnum, unw_word_t* valp, int write, void* arg) {
  auto Unwind = (NAMESPACE::Unwinder*)arg;

  if (write == 0) {
    *valp = Unwind->GetReg(regnum);
  } else {
    ERROR_AND_DIE_FMT("Can't set reg");
  }
  return 0;
}
int access_fpreg(unw_addr_space_t as, unw_regnum_t regnum, unw_fpreg_t* fpvalp, int write, void* arg) {
  // Unused.
  return 0;
}
int resume(unw_addr_space_t as, unw_cursor_t* cp, void* arg) {
  // Unused.
  return 0;
}
int get_proc_name(unw_addr_space_t as, unw_word_t addr, char* bufp, size_t buf_len, unw_word_t* offp, void* arg) {
  auto Unwind = (NAMESPACE::Unwinder*)arg;

  auto Mapping = Unwind->GetFileMapping(addr);
  if (!Mapping) {
    fextl::fmt::print(stderr, "Nope\n");
    return -UNW_EUNSPEC;
  }

  ///< TODO: If we mapped and parsed ELF files then we can get function definitions here.

  if (Mapping->Path.empty()) {
    strncpy(bufp, "GuestJIT", buf_len);
  } else {
    strncpy(bufp, FHU::Filesystem::GetFilename(Mapping->Path).c_str(), buf_len);
  }

  *offp = (addr - Mapping->Begin);
  return 0;
}
} // namespace NAMESPACE
