#include "VDSO_Emulation.h"
#include "FEXCore/IR/IR.h"
#include "Tests/LinuxSyscalls/x32/Types.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <filesystem>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

namespace FEX::VDSO {
  using TimeType = decltype(::time)*;
  using GetTimeOfDayType = decltype(::gettimeofday)*;
  using ClockGetTimeType = decltype(::clock_gettime)*;
  using ClockGetResType = decltype(::clock_getres)*;
  using GetCPUType = decltype(FHU::Syscalls::getcpu)*;

  TimeType TimePtr = ::time;
  GetTimeOfDayType GetTimeOfDayPtr = ::gettimeofday;
  ClockGetTimeType ClockGetTimePtr = ::clock_gettime;
  ClockGetResType ClockGetResPtr = ::clock_getres;
  GetCPUType GetCPUPtr = FHU::Syscalls::getcpu;

  FEXCore::Context::VDSOSigReturn VDSOPointers{};
  static void time(void* ArgsRV) {
    struct ArgsRV_t {
      time_t *a_0;
      uint64_t rv;
    } *args = reinterpret_cast<ArgsRV_t*>(ArgsRV);

    args->rv = TimePtr(args->a_0);
  }

  static void gettimeofday(void* ArgsRV) {
    struct ArgsRV_t {
      struct timeval *tv;
      struct timezone *tz;
      uint64_t rv;
    } *args = reinterpret_cast<ArgsRV_t*>(ArgsRV);

    args->rv = GetTimeOfDayPtr(args->tv, args->tz);
  }

  static void clock_gettime(void* ArgsRV) {
    struct ArgsRV_t {
      clockid_t clk_id;
      struct timespec *tp;
      uint64_t rv;
    } *args = reinterpret_cast<ArgsRV_t*>(ArgsRV);

    args->rv = ClockGetTimePtr(args->clk_id, args->tp);
  }

  static void clock_getres(void* ArgsRV) {
    struct ArgsRV_t {
      clockid_t clk_id;
      struct timespec *tp;
      uint64_t rv;
    } *args = reinterpret_cast<ArgsRV_t*>(ArgsRV);

    args->rv = ClockGetResPtr(args->clk_id, args->tp);
  }

  static void getcpu(void* ArgsRV) {
    struct ArgsRV_t {
      uint32_t *cpu;
      uint32_t *node;
      uint64_t rv;
    } *args = reinterpret_cast<ArgsRV_t*>(ArgsRV);

    args->rv = GetCPUPtr(args->cpu, args->node);
  }

  namespace x32 {
    static void time(void* ArgsRV) {
      struct ArgsRV_t {
        HLE::x32::compat_ptr<FEX::HLE::x32::old_time32_t> a_0;
        int rv;
      } *args = reinterpret_cast<ArgsRV_t*>(ArgsRV);

      time_t Host{};
      args->rv = TimePtr(&Host);
      if (args->a_0) {
        *args->a_0 = Host;
      }
    }

    static void gettimeofday(void* ArgsRV) {
      struct ArgsRV_t {
        HLE::x32::compat_ptr<FEX::HLE::x32::timeval32> tv;
        HLE::x32::compat_ptr<struct timezone> tz;
        int rv;
      } *args = reinterpret_cast<ArgsRV_t*>(ArgsRV);

      struct timeval tv64{};
      struct timeval *tv_ptr{};
      if (args->tv) {
        tv_ptr = &tv64;
      }

      args->rv = GetTimeOfDayPtr(tv_ptr, args->tz);

      if (args->tv) {
        *args->tv = tv64;
      }
    }

    static void clock_gettime(void* ArgsRV) {
      struct ArgsRV_t {
        clockid_t clk_id;
        HLE::x32::compat_ptr<HLE::x32::timespec32> tp;
        int rv;
      } *args = reinterpret_cast<ArgsRV_t*>(ArgsRV);

      struct timespec tp64{};
      args->rv = ClockGetTimePtr(args->clk_id, &tp64);

      if (args->tp) {
        *args->tp = tp64;
      }
    }

    static void clock_gettime64(void* ArgsRV) {
      struct ArgsRV_t {
        clockid_t clk_id;
        HLE::x32::compat_ptr<struct timespec> tp;
        int rv;
      } *args = reinterpret_cast<ArgsRV_t*>(ArgsRV);

      args->rv = ClockGetTimePtr(args->clk_id, args->tp);
    }

    static void clock_getres(void* ArgsRV) {
      struct ArgsRV_t {
        clockid_t clk_id;
        HLE::x32::compat_ptr<HLE::x32::timespec32> tp;
        int rv;
      } *args = reinterpret_cast<ArgsRV_t*>(ArgsRV);

      struct timespec tp64{};

      args->rv = ClockGetResPtr(args->clk_id, &tp64);

      if (args->tp) {
        *args->tp = tp64;
      }
    }

    static void getcpu(void* ArgsRV) {
      struct ArgsRV_t {
        HLE::x32::compat_ptr<uint32_t> cpu;
        HLE::x32::compat_ptr<uint32_t> node;
        int rv;
      } *args = reinterpret_cast<ArgsRV_t*>(ArgsRV);

      args->rv = GetCPUPtr(args->cpu, args->node);
    }
  }

  void LoadHostVDSO() {

    void *vdso = dlopen("linux-vdso.so.1", RTLD_LAZY | RTLD_LOCAL | RTLD_NOLOAD);
    if (!vdso) {
      vdso = dlopen("linux-gate.so.1", RTLD_LAZY | RTLD_LOCAL | RTLD_NOLOAD);
    }

    if (!vdso) {
      // We couldn't load VDSO, fallback to C implementations. Which will still be faster than emulated libc versions.
      LogMan::Msg::IFmt("linux-vdso implementation falling back to libc. Consider enabling VDSO in your kernel.");
      return;
    }

    auto SymbolPtr = dlsym(vdso, "__vdso_time");
    if (SymbolPtr) {
      TimePtr = reinterpret_cast<TimeType>(SymbolPtr);
    }

    SymbolPtr = dlsym(vdso, "__vdso_gettimeofday");
    if (SymbolPtr) {
      GetTimeOfDayPtr = reinterpret_cast<GetTimeOfDayType>(SymbolPtr);
    }

    SymbolPtr = dlsym(vdso, "__vdso_clock_gettime");
    if (SymbolPtr) {
      ClockGetTimePtr = reinterpret_cast<ClockGetTimeType>(SymbolPtr);
    }

    SymbolPtr = dlsym(vdso, "__vdso_clock_getres");
    if (SymbolPtr) {
      ClockGetResPtr = reinterpret_cast<ClockGetResType>(SymbolPtr);
    }

    SymbolPtr = dlsym(vdso, "__vdso_getcpu");
    if (SymbolPtr) {
      GetCPUPtr = reinterpret_cast<GetCPUType>(SymbolPtr);
    }
    dlclose(vdso);
  }

  static std::vector<FEXCore::IR::ThunkDefinition> VDSODefinitions = {
    {
        // sha256(libVDSO:time)
        { 0x37, 0x63, 0x46, 0xb0, 0x79, 0x06, 0x5f, 0x9d, 0x00, 0xb6, 0x8d, 0xfd, 0x9e, 0x4a, 0x62, 0xcd, 0x1e, 0x6c, 0xcc, 0x22, 0xcd, 0xb2, 0xc0, 0x17, 0x7d, 0x42, 0x6a, 0x40, 0xd1, 0xeb, 0xfa, 0xe0 },
        nullptr,
    },
    {
        // sha256(libVDSO:gettimeofday)
        { 0x77, 0x2a, 0xde, 0x1c, 0x13, 0x2d, 0xe9, 0x48, 0xaf, 0xe0, 0xba, 0xcc, 0x6a, 0x89, 0xff, 0xca, 0x4a, 0xdc, 0xd5, 0x63, 0x2c, 0xc5, 0x62, 0x8b, 0x5d, 0xde, 0x0b, 0x15, 0x35, 0xc6, 0xc7, 0x14 },
        nullptr,
    },
    {
        // sha256(libVDSO:clock_gettime)
        { 0x3c, 0x96, 0x9b, 0x2d, 0xc3, 0xad, 0x2b, 0x3b, 0x9c, 0x4e, 0x4d, 0xca, 0x1c, 0xe8, 0x18, 0x4a, 0x12, 0x8a, 0xe4, 0xc1, 0x56, 0x92, 0x73, 0xce, 0x65, 0x85, 0x5f, 0x65, 0x7e, 0x94, 0x26, 0xbe },
        nullptr,
    },

    {
        // sha256(libVDSO:clock_gettime64)
        { 0xba, 0xe9, 0x6d, 0x30, 0xc0, 0x68, 0xc6, 0xd7, 0x59, 0x04, 0xf7, 0x10, 0x06, 0x72, 0x88, 0xfd, 0x4c, 0x57, 0x0f, 0x31, 0xa5, 0xea, 0xa9, 0xb9, 0xd3, 0x8d, 0x03, 0x81, 0x50, 0x16, 0x22, 0x71 },
        nullptr,
    },

    {
        // sha256(libVDSO:clock_getres)
        { 0xe4, 0xa1, 0xf6, 0x23, 0x35, 0xae, 0xb7, 0xb6, 0xb0, 0x37, 0xc5, 0xc3, 0xa3, 0xfd, 0xbf, 0xa2, 0xa1, 0xc8, 0x95, 0x78, 0xe5, 0x76, 0x86, 0xdb, 0x3e, 0x6c, 0x54, 0xd5, 0x02, 0x60, 0xd8, 0x6d },
        nullptr,
    },
    {
        // sha256(libVDSO:getcpu)
        { 0x39, 0x83, 0x39, 0x36, 0x0f, 0x68, 0xd6, 0xfc, 0xc2, 0x3a, 0x97, 0x11, 0x85, 0x09, 0xc7, 0x25, 0xbb, 0x50, 0x49, 0x55, 0x6b, 0x0c, 0x9f, 0x50, 0x37, 0xf5, 0x9d, 0xb0, 0x38, 0x58, 0x57, 0x12 },
        nullptr,
    },
  };

  void LoadGuestVDSOSymbols(bool Is64Bit, char *VDSOBase) {
    // We need to load symbols we care about.
    if (Is64Bit) {
      // We don't care about any 64-bit symbols right now.
      return;
    }

    // 32-bit symbol loading.
    const Elf32_Ehdr *Header = reinterpret_cast<const Elf32_Ehdr*>(VDSOBase);

    // First walk the section headers to find the symbol table.
    const Elf32_Shdr *RawShdrs =
      reinterpret_cast<const Elf32_Shdr *>(VDSOBase + Header->e_shoff);

    const Elf32_Shdr *StrHeader = &RawShdrs[Header->e_shstrndx];
    char const *SHStrings = VDSOBase + StrHeader->sh_offset;

    const Elf32_Shdr *SymTableHeader{};
    const Elf32_Shdr *StringTableHeader{};

    for (size_t i = 0; i < Header->e_shnum; ++i) {
      const auto &Header = RawShdrs[i];
      if (Header.sh_type == SHT_SYMTAB && strcmp(&SHStrings[Header.sh_name], ".symtab") == 0) {
        SymTableHeader = &Header;
        StringTableHeader = &RawShdrs[SymTableHeader->sh_link];
        break;
      }
    }

    if (!SymTableHeader) {
      // Couldn't find symbol table
      return;
    }

    char const *StrTab = VDSOBase + StringTableHeader->sh_offset;
    size_t NumSymbols = SymTableHeader->sh_size / SymTableHeader->sh_entsize;

    for (size_t i = 0; i < NumSymbols; ++i) {
      uint64_t offset = SymTableHeader->sh_offset + i * SymTableHeader->sh_entsize;
      Elf32_Sym const *Symbol =
          reinterpret_cast<Elf32_Sym const *>(VDSOBase + offset);
      if (ELF32_ST_VISIBILITY(Symbol->st_other) != STV_HIDDEN &&
          Symbol->st_value != 0) {
        char const * Name = &StrTab[Symbol->st_name];
        if (Name[0] != '\0') {
          if (strcmp(Name, "__kernel_sigreturn") == 0) {
            VDSOPointers.VDSO_kernel_sigreturn = VDSOBase + Symbol->st_value;
          }
          else if (strcmp(Name, "__kernel_rt_sigreturn") == 0) {
            VDSOPointers.VDSO_kernel_rt_sigreturn = VDSOBase + Symbol->st_value;
          }
        }
      }
    }
  }

  void* LoadVDSOThunks(bool Is64Bit, MapperFn Mapper) {
    void* VDSOBase{};
    FEX_CONFIG_OPT(ThunkGuestLibs, THUNKGUESTLIBS);
    FEX_CONFIG_OPT(ThunkGuestLibs32, THUNKGUESTLIBS32);

    std::filesystem::path ThunkGuestPath{};
    if (Is64Bit) {
      ThunkGuestPath = std::filesystem::path(ThunkGuestLibs()) / "libVDSO-guest.so";

      // Set the Thunk definition pointers for x86-64
      VDSODefinitions[0].ThunkFunction = &FEX::VDSO::time;
      VDSODefinitions[1].ThunkFunction = &FEX::VDSO::gettimeofday;
      VDSODefinitions[2].ThunkFunction = &FEX::VDSO::clock_gettime;
      VDSODefinitions[3].ThunkFunction = &FEX::VDSO::clock_gettime;
      VDSODefinitions[4].ThunkFunction = &FEX::VDSO::clock_getres;
      VDSODefinitions[5].ThunkFunction = &FEX::VDSO::getcpu;
    }
    else {
      ThunkGuestPath = std::filesystem::path(ThunkGuestLibs32()) / "libVDSO-guest.so";

      // Set the Thunk definition pointers for x86
      VDSODefinitions[0].ThunkFunction = &FEX::VDSO::x32::time;
      VDSODefinitions[1].ThunkFunction = &FEX::VDSO::x32::gettimeofday;
      VDSODefinitions[2].ThunkFunction = &FEX::VDSO::x32::clock_gettime;
      VDSODefinitions[3].ThunkFunction = &FEX::VDSO::x32::clock_gettime64;
      VDSODefinitions[4].ThunkFunction = &FEX::VDSO::x32::clock_getres;
      VDSODefinitions[5].ThunkFunction = &FEX::VDSO::x32::getcpu;
    }

    // Load VDSO if we can
    int VDSOFD = ::open(ThunkGuestPath.string().c_str(), O_RDONLY);

    if (VDSOFD != -1) {
      // Get file size
      size_t VDSOSize = lseek(VDSOFD, 0, SEEK_END);

      if (VDSOSize >= 4) {
        // Reset to beginning
        lseek(VDSOFD, 0, SEEK_SET);
        VDSOSize = FEXCore::AlignUp(VDSOSize, 4096);

        // Map the VDSO file to memory
        VDSOBase = Mapper(nullptr, VDSOSize, PROT_READ, MAP_PRIVATE, VDSOFD, 0);

        // Since we found our VDSO thunk library, find our host VDSO function implementations.
        LoadHostVDSO();

      }
      close(VDSOFD);
      LoadGuestVDSOSymbols(Is64Bit, reinterpret_cast<char*>(VDSOBase));
    }

    return VDSOBase;
  }

  uint64_t GetVSyscallEntry(const void* VDSOBase) {
    if (!VDSOBase) {
      return 0;
    }

    // Extract the vsyscall location from the VDSO header.
    auto Header = reinterpret_cast<const Elf32_Ehdr*>(VDSOBase);

    if (Header->e_entry) {
      return reinterpret_cast<uint64_t>(VDSOBase) + Header->e_entry;
    }

    return 0;
  }

  std::vector<FEXCore::IR::ThunkDefinition> const& GetVDSOThunkDefinitions() {
    return VDSODefinitions;
  }

  FEXCore::Context::VDSOSigReturn const& GetVDSOSymbols() {
    return VDSOPointers;
  }
}
