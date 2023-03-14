
#pragma once

#include "Common/Config.h"
#include "Common/FDUtils.h"
#include "FEXCore/Utils/Allocator.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/VDSO_Emulation.h"
#include "Linux/Utils/ELFParser.h"
#include "Linux/Utils/ELFSymbolDatabase.h"

#include <array>
#include <bitset>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <list>
#include <random>
#include <string>

#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Core/UContext.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXHeaderUtils/Syscalls.h>
#include <FEXHeaderUtils/TypeDefines.h>
#include <FEXHeaderUtils/SymlinkChecks.h>

#include <elf.h>
#include <fcntl.h>
#include <fmt/format.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <sys/personality.h>
#include <sys/random.h>

#define PAGE_START(x) ((x) & ~(uintptr_t)(4095))
#define PAGE_OFFSET(x) ((x) & 4095)
#define PAGE_ALIGN(x) (((x) + 4095) & ~(uintptr_t)(4095))

class ELFCodeLoader2 final : public FEXCore::CodeLoader {
  ELFParser MainElf;
  ELFParser InterpElf;

  bool ElfValid {false};
  bool ExecutableStack {false};
  uintptr_t MainElfBase;
  uintptr_t InterpeterElfBase;
  uintptr_t MainElfEntrypoint;
  uintptr_t Entrypoint;
  uintptr_t BrkStart;
  uintptr_t StackPointer;

  size_t CalculateTotalElfSize(const std::vector<Elf64_Phdr> &headers)
  {
    auto first = std::find_if(headers.begin(), headers.end(), [](const Elf64_Phdr &Header) { return Header.p_type == PT_LOAD; });
    auto last = std::find_if(headers.rbegin(), headers.rend(), [](const Elf64_Phdr &Header) { return Header.p_type == PT_LOAD; });

    if (first == headers.end())
      return 0;

    return PAGE_ALIGN(last->p_vaddr + last->p_memsz);
  }

  template<typename T>
  bool MapFile(const ELFParser& file, uintptr_t Base, const Elf64_Phdr &Header, int prot, int flags, T Mapper) {

    auto addr = Base + PAGE_START(Header.p_vaddr);
    auto size = Header.p_filesz + PAGE_OFFSET(Header.p_vaddr);
    auto off = Header.p_offset - PAGE_OFFSET(Header.p_vaddr);

    size = PAGE_ALIGN(size);
    if (size == 0) {
      // PT_LOAD section without a file size
      // Will need to have a memory size that is not zero instead
      return true;
    }

    void *rv = Mapper((void*)addr, size, prot, flags, file.fd, off);

    if (rv == MAP_FAILED) {
      // uhoh, something went wrong
      LogMan::Msg::EFmt("MapFile: Some elf mapping failed, {}, fd: {}\n", errno, file.fd);
      return false;
    } else {
      auto Filename = FEX::get_fdpath(file.fd);
      if (Filename.has_value()) {
        Sections.push_back({Base, (uintptr_t)rv, size, (off_t)off, Filename.value(), (prot & PROT_EXEC) != 0});
      }

      return true;
    }
  }

  int MapFlags(const Elf64_Phdr &Header) {
    int rv = 0;

    if (Header.p_flags & PF_R)
      rv |= PROT_READ;

    if (Header.p_flags & PF_W)
      rv |= PROT_WRITE;

    if (Header.p_flags & PF_X)
      rv |= PROT_EXEC;

    return rv;
  }

  template <typename TMap, typename TUnmap>
  std::optional<uintptr_t> LoadElfFile(ELFParser& Elf, uintptr_t *BrkBase, TMap Mapper, TUnmap Unmapper, uint64_t LoadHint = 0) {

    uintptr_t LoadBase = 0;

    if (BrkBase) {
      *BrkBase = 0;
    }

    if (Elf.ehdr.e_type == ET_DYN) {
      // needs base address
      auto TotalSize = CalculateTotalElfSize(Elf.phdrs) + (BrkBase ? BRK_SIZE : 0);
      LoadBase = (uintptr_t)Mapper(reinterpret_cast<void*>(LoadHint), TotalSize, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
      if ((void*)LoadBase == MAP_FAILED) {
        return {};
      }

      //fprintf(stderr, "elf %d: %lx-%lx\n", Elf.fd, LoadBase, LoadBase + TotalSize);
      if (BrkBase) {
        *BrkBase = LoadBase + (TotalSize - BRK_SIZE);
      }
    }

    for(const auto &Header: Elf.phdrs) {
      if (Header.p_type != PT_LOAD)
        continue;

      int MapProt = MapFlags(Header);
      int MapType = MAP_PRIVATE | MAP_DENYWRITE | MAP_FIXED;

      if (!MapFile(Elf, LoadBase, Header, MapProt, MapType, Mapper)) {
        return {};
      }

      if (Header.p_memsz > Header.p_filesz) {
        // clear bss
        auto BSSStart = LoadBase + Header.p_vaddr + Header.p_filesz;
        auto BSSPageStart = PAGE_ALIGN(BSSStart);
        auto BSSPageEnd = PAGE_ALIGN(LoadBase + Header.p_vaddr + Header.p_memsz);

        // Only clear padding bytes if the section is writable
        if (Header.p_flags & PF_W) {
          memset((void*)BSSStart, 0, BSSPageStart - BSSStart);
        }

        if (BSSPageStart != BSSPageEnd) {
          auto bss = Mapper((void*)BSSPageStart, BSSPageEnd - BSSPageStart, MapProt, MapType | MAP_ANONYMOUS, -1, 0);
          if ((void*)bss == MAP_FAILED) {
            LogMan::Msg::EFmt("Failed to allocate BSS @ {}, {}\n", fmt::ptr(bss), errno);
            return {};
          }
        }
      }

      if (BrkBase) {
        // Keep track of highest address for BRK
        auto memend = LoadBase + Header.p_vaddr + Header.p_memsz;

        // track elf_brk
        if (memend > *BrkBase) {
          *BrkBase = PAGE_ALIGN(memend);
        }
      }
    }

    return LoadBase;
  }

  static bool GetRandom(void *Data, size_t DataSize) {
    ssize_t Result{};
    do {
      // This is guaranteed to not be interrupted by a signal,
      // since fewer than 256 bytes of RNG data are requested
      Result = getrandom(Data, DataSize, 0);
    } while (Result != -1 && Result != DataSize);

    return Result != -1;
  }

  public:

  static std::string ResolveRootfsFile(std::string const &File, std::string RootFS) {
    // If the path is relative then just run that
    if (File[0] != '/') {
      return File;
    }

    std::string RootFSLink = RootFS + File;

    char Filename[PATH_MAX];
    while(FHU::Symlinks::IsSymlink(RootFSLink)) {
      // Do some special handling if the RootFS's linker is a symlink
      // Ubuntu's rootFS by default provides an absolute location symlink to the linker
      // Resolve this around back to the rootfs
      auto SymlinkPath = FHU::Symlinks::ResolveSymlink(RootFSLink, Filename);
      if (SymlinkPath.starts_with('/')) {
        RootFSLink = RootFS;
        RootFSLink += SymlinkPath;
      }
      else {
        break;
      }
    }

    return RootFSLink;
  }

  struct LoadedSection {
    uintptr_t ElfBase;
    uintptr_t Base;
    size_t Size;
    off_t Offs;
    std::string Filename;
    bool Executable;
  };

  std::vector<LoadedSection> Sections;

  ELFCodeLoader2(std::string const &Filename, const std::string_view FEXFDString, std::string const &RootFS, [[maybe_unused]] std::vector<std::string> const &args, std::vector<std::string> const &ParsedArgs, char **const envp = nullptr, FEXCore::Config::Value<std::string> *AdditionalEnvp = nullptr) :
    Args {args} {

    bool LoadedWithFD = false;
    int FD = getauxval(AT_EXECFD);

    if (!FEXFDString.empty()) {
      // If we passed the execve FD to us then use that.
      const char *StartPtr = FEXFDString.data();
      char *EndPtr{};
      FD = ::strtol(StartPtr, &EndPtr, 10);
      if (EndPtr == StartPtr) {
        LogMan::Msg::AFmt("FEXInterpreter passed invalid FD to exececute: {}", FEXFDString);
        return;
      }
      unsetenv("FEX_EXECVEFD");
    }

    // If we are provided an EXECFD then attempt to execute that first
    // This happens in the case of binfmt_misc usage
    if (FD != 0) {
      if (!MainElf.ReadElf(FD)) {
        return;
      }
      LoadedWithFD = true;
    }
    else {
      if (!MainElf.ReadElf(ResolveRootfsFile(Filename, RootFS)) && !MainElf.ReadElf(Filename)) {
        return;
      }
    }

    // If we have loaded with EXECFD then we have binfmt_misc preserve argv[0] also set
    // This adds an additional argument to our argument list that we need to ignore
    // argv[0] = FEXInterpreter
    // argv[1] = <Path to binary>
    // argv[2] = <original user typed path to binary>
    // If our kernel if v5.12 or higher then
    // We can check if this exists by checking auxv[AT_FLAGS] for AT_FLAGS_PRESERVE_ARGV0
    // Else we need to make an assumption that if we were loaded with FD that we have preserve enabled

    uint64_t AtFlags = getauxval(AT_FLAGS);
#ifndef AT_FLAGS_PRESERVE_ARGV0
#define AT_FLAGS_PRESERVE_ARGV0 1
#endif
    uint32_t HostKernel = FEX::HLE::SyscallHandler::CalculateHostKernelVersion();
    if ((HostKernel >= FEX::HLE::SyscallHandler::KernelVersion(5, 12, 0) &&
         (AtFlags & AT_FLAGS_PRESERVE_ARGV0)) ||
       LoadedWithFD){

      // Erase the initial argument from the list in this case
      Args.erase(Args.begin());
    }

    // Append any additional arguments from config
    for (auto &Arg : AdditionalArguments.All()) {
      Args.emplace_back(Arg);
    }

    if (!MainElf.InterpreterElf.empty()) {
      if (!InterpElf.ReadElf(ResolveRootfsFile(MainElf.InterpreterElf, RootFS)) && !InterpElf.ReadElf(MainElf.InterpreterElf))
        return;

      if (!InterpElf.InterpreterElf.empty())
        return;

      if (InterpElf.type != MainElf.type)
        return;
    }

    ElfValid = true;

    if (!!envp) {
      // If we had envp passed in then make sure to set it up on the guest
      for (unsigned i = 0;; ++i) {
        if (envp[i] == nullptr)
          break;
        EnvironmentVariables.emplace_back(envp[i]);
      }
    }

    if (!!AdditionalEnvp) {
      auto EnvpList = AdditionalEnvp->All();
      for (auto iter = EnvpList.begin(); iter != EnvpList.end(); ++iter) {
        EnvironmentVariables.emplace_back(*iter);
      }
    }

    if (InjectLibSegFault()) {
      EnvironmentVariables.emplace_back("LD_PRELOAD=libSegFault.so");
    }

    // Calculate argument and envp backing sizes
    for (unsigned i = 0; i < Args.size(); ++i) {
      ArgumentBackingSize += Args[i].size() + 1;
    }

    for (unsigned i = 0; i < EnvironmentVariables.size(); ++i) {
      EnvironmentBackingSize += EnvironmentVariables[i].size() + 1;
    }

    for (auto &Arg : ParsedArgs) {
      LoaderArgs.emplace_back(Arg.c_str());
    }
  }

  void FreeSections() {
    Sections.clear();
  }

  virtual uint64_t StackSize() const override { return STACK_SIZE; }
  virtual uint64_t GetStackPointer() override { return StackPointer; }
  virtual uint64_t DefaultRIP() const override { return Entrypoint; };

  struct auxv32_t {
    uint32_t key;
    uint32_t val;
  };

  struct auxv_t {
    uint64_t key;
    uint64_t val;
  };

  bool MapMemory(const MapperFn& Mapper, const UnmapperFn& Unmapper) override {
    for (auto Header: MainElf.phdrs) {
      if (Header.p_type == PT_GNU_STACK) {
        if (Header.p_flags & PF_X)
          ExecutableStack = true;
      }

      // We ignore LOPROC..HIPROC here, kernel has a platform specific hook about it
      // Both for the main and the interpreter elf
    }

    // Set the process personality here
    // This needs some more investigation
    // READ_IMPLIES_EXEC might be default for 32-bit elfs
    // Also, what about ADDR_LIMIT_3GB & co ?
    if (-1 == personality(PER_LINUX | (ExecutableStack ? READ_IMPLIES_EXEC : 0))) {
      LogMan::Msg::EFmt("Setting personality failed");
      return false;
    }

    // What about ASLR and such ?
    // ADDR_LIMIT_3GB STACK -> 0xc0000000 else -> 0xFFFFe000

    // map stack here, so that nothing gets mapped there
    // This works with both 64-bit and 32-bit. The mapper will only give us a function in the correct region
    //
    // MAP_GROWSDOWN is required here. The default stack pointer allocated by the kernel is mapped with it.
    // Some libraries (like libfmod) will have a PT_GNU_STACK with executable stack bit set
    // On dlopen glibc will check its current stack allocation permission bits (using internal expectations of allocation, not /proc/self/maps)
    // If stack hasn't been allocated as executable then it will proceed to mprotect the range with the executable bit set
    // Then it will mprotect the base stack page with `PROT_READ|PROT_WRITE|PROT_EXEC|PROT_GROWSDOWN`
    // If the original stack memory region wasn't allocated with MAP_GROWSDOWN then the mprotect with PROT_GROWSDOWN will fail with EINVAL
    //
    // This is still technically a memory leak if the stack grows, but since the primary thread's stack only gets destroyed on process close, this is
    // fine.

    // Stacks need to be allocated at the hint location just like on a real x86 system.
    // These are 128MB regions on both x86-64 and x86.
    //
    // These are required to be in the correct location taking up the appropriate 128MB of space, otherwise the wine preloader crashes FEX.
    // This is due to the wine-preloader hardcoding addresses [0x7FFFFE000000 - 0x7FFFFFFF0000) as a top-down
    // allocation region. They use mmap with MAP_FIXED, ignoring any previously mapped area at that location and overwriting it.
    // Wine-preloader is expecting to allocate 32MB out of the total 128MB stack space in this case. Leaving 96MB for the application.
    //
    // If FEX doesn't allocate the stack in this region (nullptr mmap hint) then later allocations that FEX does will /eventually/
    // end up inside of this address space that wine allocates. This usually ends up being a JIT CodeBuffer, which zeroes the memory and faults with a
    // SIGILL.
    //
    // On the upside, this more accurately emulates how the kernel allocates stack space for the application when hinting at the location.
    //
    void* StackPointerBase{};
    uint64_t StackHint = Is64BitMode() ? STACK_HINT_64 : STACK_HINT_32;

    // Allocate the base of the full 128MB stack range.
    StackPointerBase = Mapper(reinterpret_cast<void*>(StackHint), FULL_STACK_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK | MAP_GROWSDOWN | MAP_NORESERVE, -1, 0);

    if (StackPointerBase == reinterpret_cast<void*>(~0ULL)) {
      LogMan::Msg::EFmt("Allocating stack failed");
      return false;
    }

    // Allocate with permissions the 8MB of regular stack size.
    StackPointer = reinterpret_cast<uintptr_t>(Mapper(
      reinterpret_cast<void*>(reinterpret_cast<uint64_t>(StackPointerBase) + FULL_STACK_SIZE - StackSize()),
      StackSize(), PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK | MAP_GROWSDOWN, -1, 0));

    if (StackPointer == ~0ULL) {
      LogMan::Msg::EFmt("Allocating stack failed");
      return false;
    }

    // Load the interpreter ELF first.
    // This allows the top-down allocation of the kernel to put this at the top of the VA space.
    // This matches behaviour of native execution more closely.
    //
    // eg:
    // 555555554000-555555558000 r--p 00000000 103:0a 1311400                   /usr/bin/ls
    // 555555558000-55555556c000 r-xp 00004000 103:0a 1311400                   /usr/bin/ls
    // 55555556c000-555555574000 r--p 00018000 103:0a 1311400                   /usr/bin/ls
    // 555555575000-555555577000 rw-p 00020000 103:0a 1311400                   /usr/bin/ls
    // 555555577000-555555578000 rw-p 00000000 00:00 0                          [heap]
    // 7ffff7fbb000-7ffff7fbd000 rw-p 00000000 00:00 0
    // 7ffff7fbd000-7ffff7fc1000 r--p 00000000 00:00 0                          [vvar]
    // 7ffff7fc1000-7ffff7fc3000 r-xp 00000000 00:00 0                          [vdso]
    // 7ffff7fc3000-7ffff7fc5000 r--p 00000000 103:0a 1316948                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    // 7ffff7fc5000-7ffff7fef000 r-xp 00002000 103:0a 1316948                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    // 7ffff7fef000-7ffff7ffa000 r--p 0002c000 103:0a 1316948                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    // 7ffff7ffb000-7ffff7fff000 rw-p 00037000 103:0a 1316948                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    // 7ffffffdd000-7ffffffff000 rw-p 00000000 00:00 0                          [stack]
    // ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
    //
    // ARM:
    // 55ccaf8b1000-55ccaf8b5000 r--p 00000000 00:2a 4                          /tmp/.FEXMount178532-oiFrTF/usr/bin/ls
    // 55ccaf8b5000-55ccaf8c9000 r-xp 00004000 00:2a 4                          /tmp/.FEXMount178532-oiFrTF/usr/bin/ls
    // 55ccaf8c9000-55ccaf8d1000 r--p 00018000 00:2a 4                          /tmp/.FEXMount178532-oiFrTF/usr/bin/ls
    // 55ccaf8d1000-55ccaf8d2000 ---p 00000000 00:00 0
    // 55ccaf8d2000-55ccaf8d4000 rw-p 00020000 00:2a 4                          /tmp/.FEXMount178532-oiFrTF/usr/bin/ls
    // 55ccaf8d4000-55ccb00d5000 rw-p 00000000 00:00 0
    // <... Snip of misc allocations ...>
    // 7fffff6c2000-7fffff6c4000 r--p 00000000 00:2a 22                         /tmp/.FEXMount178532-oiFrTF/usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    // 7fffff6c4000-7fffff6ee000 r-xp 00002000 00:2a 22                         /tmp/.FEXMount178532-oiFrTF/usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    // 7fffff6ee000-7fffff6f9000 r--p 0002c000 00:2a 22                         /tmp/.FEXMount178532-oiFrTF/usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    // 7fffff6f9000-7fffff6fa000 ---p 00000000 00:00 0
    // 7fffff6fa000-7fffff6fe000 rw-p 00037000 00:2a 22                         /tmp/.FEXMount178532-oiFrTF/usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    // 7fffff7fe000-7fffffffe000 rw-p 00000000 00:00 0
    // 7fffffffe000-7ffffffff000 r--p 00000000 08:82 7082611                    /usr/share/fex-emu/GuestThunks/libVDSO-guest.so
    // 7ffffffff000-800000000000 rw-p 00000000 00:00 0
    uint64_t ELFLoadHint = 0;

    if (!MainElf.InterpreterElf.empty()) {
      uint64_t InterpLoadBase = 0;
      if (auto elf = LoadElfFile(InterpElf, nullptr, Mapper, Unmapper)) {
        InterpLoadBase = *elf;
      } else {
        LogMan::Msg::EFmt("Failed to load interpreter elf file");
        return false;
      }

      InterpeterElfBase = InterpLoadBase + InterpElf.phdrs.front().p_vaddr - InterpElf.phdrs.front().p_offset;
      Entrypoint = InterpLoadBase + InterpElf.ehdr.e_entry;

      // If the ELF has an interpreter and is dynamic then we should provide a address hint for loading.
      // The kernel calculates this `load_bias` by dividing the task size by three then multiplying by two.
      // It then also offsets by a random number for ASLR purposes.
      //
      // Random number that gets added to the base needs to be in the number of bits (multiplied by pages):
      // 64-bit: [28, 32] bits
      // 32-bit: [8, 16] bits
      // By default the /minimum/ number of bits is used here.
      constexpr uint64_t TASK_SIZE_64 = (1ULL << 47);
      constexpr uint64_t TASK_SIZE_32 = (1ULL << 32);
      if (Is64BitMode()) {
        // Ensure that if we are running on a 36-bit VA system, we don't try hinting that an ELF should
        // live way outside the VA space.
        uint64_t HostVASize = 1ULL << FEXCore::Allocator::DetermineVASize();
        ELFLoadHint = std::min(HostVASize, TASK_SIZE_64) / 3 * 2;
      }
      else {
        ELFLoadHint = TASK_SIZE_32 / 3 * 2;
      }
#define ASLR_LOAD
#ifdef ASLR_LOAD
      // Only enable ASLR randomization if the personality has it enabled.
      uint32_t Personality = personality(~0ULL);
      bool NoRandomize = (Personality & ADDR_NO_RANDOMIZE) == ADDR_NO_RANDOMIZE;

      if (!NoRandomize) {
        constexpr uint64_t ASLR_BITS_64 = 28;
        constexpr uint64_t ASLR_BITS_32 = 8;
        uint64_t ASLR_Offset{};
        if (!GetRandom(&ASLR_Offset, sizeof(ASLR_Offset))) {
          // getrandom failed for some reason.
          ASLR_Offset = 0;
          LogMan::Msg::EFmt("RNG failed. ASLR will not work.");
        }

        if (Is64BitMode()) {
          ASLR_Offset &= (1ULL << ASLR_BITS_64) - 1;
        }
        else {
          ASLR_Offset &= (1ULL << ASLR_BITS_32) - 1;
        }

        ASLR_Offset <<= FHU::FEX_PAGE_SHIFT;
        ELFLoadHint += ASLR_Offset;
      }
#endif
      // Align the mapping
      ELFLoadHint &= FHU::FEX_PAGE_MASK;
    }

    // load the main elf

    uintptr_t BrkBase = 0;

    uintptr_t LoadBase = 0;

    if (auto elf = LoadElfFile(MainElf, &BrkBase, Mapper, Unmapper, ELFLoadHint)) {
      LoadBase = *elf;
      if (MainElf.ehdr.e_type == ET_DYN) {
        BaseOffset = LoadBase;
      }
    } else {
      LogMan::Msg::EFmt("Failed to load elf file");
      return false;
    }

    // XXX Randomise brk?

    BrkStart = (uint64_t)Mapper((void*)BrkBase, BRK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);

    if ((void*)BrkStart == MAP_FAILED) {
      LogMan::Msg::EFmt("Failed to allocate BRK @ {:x}, {}\n", BrkBase, errno);
      return false;
    }

    MainElfBase = LoadBase + MainElf.phdrs.front().p_vaddr - MainElf.phdrs.front().p_offset;
    MainElfEntrypoint = LoadBase + MainElf.ehdr.e_entry;

    if (MainElf.InterpreterElf.empty()) {
      InterpeterElfBase = 0;
      Entrypoint = MainElfEntrypoint;
    }

    // All done

    // Setup AuxVars
    AuxVariables.emplace_back(auxv_t{11, getauxval(AT_UID)}); // AT_UID
    AuxVariables.emplace_back(auxv_t{12, getauxval(AT_EUID)}); // AT_EUID
    AuxVariables.emplace_back(auxv_t{13, getauxval(AT_GID)}); // AT_GID
    AuxVariables.emplace_back(auxv_t{14, getauxval(AT_EGID)}); // AT_EGID
    AuxVariables.emplace_back(auxv_t{17, getauxval(AT_CLKTCK)}); // AT_CLKTIK
    AuxVariables.emplace_back(auxv_t{6, 0x1000}); // AT_PAGESIZE
    AuxRandom = &AuxVariables.emplace_back(auxv_t{25, ~0ULL}); // AT_RANDOM
    AuxVariables.emplace_back(auxv_t{23, getauxval(AT_SECURE)}); // AT_SECURE
    AuxVariables.emplace_back(auxv_t{8, 0}); // AT_FLAGS
    AuxVariables.emplace_back(auxv_t{5, MainElf.phdrs.size()}); // AT_PHNUM
    AuxVariables.emplace_back(auxv_t{16, HWCap}); // AT_HWCAP
    AuxVariables.emplace_back(auxv_t{26, HWCap2}); // AT_HWCAP2
    AuxVariables.emplace_back(auxv_t{51, CalculateSignalStackSize()}); // AT_MINSIGSTKSZ
    AuxPlatform = &AuxVariables.emplace_back(auxv_t{24, ~0ULL}); // AT_PLATFORM

    if (Is64BitMode()) {
      AuxVariables.emplace_back(auxv_t{4, 0x38}); // AT_PHENT
    }
    else {
      AuxVariables.emplace_back(auxv_t{4, 0x20}); // AT_PHENT

      auto VSyscallEntry = FEX::VDSO::GetVSyscallEntry(VDSOBase);
      if (!VSyscallEntry) [[unlikely]] {
        // If the VDSO thunk doesn't exist then we might not have a vsyscall entry.
        // Newer glibc requires vsyscall to exist now. So let's allocate a buffer and stick a vsyscall in to it.
        auto VSyscallPage = Mapper(nullptr, FHU::FEX_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        constexpr static uint8_t VSyscallCode[] = {
          0xcd, 0x80, // int 0x80
          0xc3,       // ret
        };
        memcpy(VSyscallPage, VSyscallCode, sizeof(VSyscallCode));
        mprotect(VSyscallPage, FHU::FEX_PAGE_SIZE, PROT_READ);
        VSyscallEntry = reinterpret_cast<uint64_t>(VSyscallPage);
      }

      AuxVariables.emplace_back(auxv_t{32, VSyscallEntry}); // AT_SYSINFO - Entry point to syscall
    }

    if (VDSOBase) {
      AuxVariables.emplace_back(auxv_t{33, reinterpret_cast<uint64_t>(VDSOBase)}); // AT_SYSINFO_EHDR - Address of the start of VDSO
    }

    AuxVariables.emplace_back(auxv_t{3, MainElfBase + MainElf.ehdr.e_phoff}); // Program header
    AuxVariables.emplace_back(auxv_t{7, InterpeterElfBase}); // AT_BASE - Interpreter address
    AuxVariables.emplace_back(auxv_t{9, MainElfEntrypoint}); // AT_ENTRY

    AuxVariables.emplace_back(auxv_t{0, 0}); // Null ender

    SetupStack();

    // Cleanup FDs so they don't stay open
    MainElf.Closefd();
    InterpElf.Closefd();
    return true;
  }

  // Helper for stack setup
  template <typename PointerType, typename AuxType, size_t PointerSize>
  static void SetupPointers(
    uintptr_t StackPointer,
    uint64_t AuxVOffset,
    uint64_t ArgumentOffset,
    uint64_t EnvpOffset,
    const std::vector<std::string> &Args,
    const std::vector<std::string> &EnvironmentVariables,
    const std::list<auxv_t> &AuxVariables,
    uint64_t *AuxTabBase,
    uint64_t *AuxTabSize,
    PointerType RandomNumberOffset,
    PointerType PlatformNameOffset
    ) {
    // Pointer list offsets
    PointerType *ArgumentPointers = reinterpret_cast<PointerType*>(StackPointer + PointerSize);
    PointerType *PadPointers = reinterpret_cast<PointerType*>(StackPointer + PointerSize + Args.size() * PointerSize);
    PointerType *EnvpPointers = reinterpret_cast<PointerType*>(StackPointer + PointerSize + Args.size() * PointerSize + PointerSize);
    AuxType *AuxVPointers = reinterpret_cast<AuxType *>(StackPointer + AuxVOffset);

    // Arguments memory lives after everything else
    uint8_t *ArgumentBackingBase = reinterpret_cast<uint8_t*>(StackPointer + ArgumentOffset);
    uint8_t *EnvpBackingBase = reinterpret_cast<uint8_t*>(StackPointer + EnvpOffset);
    PointerType ArgumentBackingBaseGuest = StackPointer + ArgumentOffset;
    PointerType EnvpBackingBaseGuest = StackPointer + EnvpOffset;

    *reinterpret_cast<PointerType *>(StackPointer + 0) = Args.size();
    PadPointers[0] = 0;

    // If we don't have any, just make sure the first is nullptr
    EnvpPointers[0] = 0;

    uint64_t CurrentOffset = 0;
    for (size_t i = 0; i < Args.size(); ++i) {
      size_t ArgSize = Args[i].size();
      // Set the pointer to this argument
      ArgumentPointers[i] = ArgumentBackingBaseGuest + CurrentOffset;
      if (ArgSize > 0) {
        // Copy the string in to the final location
        memcpy(reinterpret_cast<void*>(ArgumentBackingBase + CurrentOffset), &Args[i].at(0), ArgSize);
      }

      // Set the null terminator for the string
      *reinterpret_cast<uint8_t*>(ArgumentBackingBase + CurrentOffset + ArgSize + 1) = 0;

      CurrentOffset += ArgSize + 1;
    }

    CurrentOffset = 0;
    for (size_t i = 0; i < EnvironmentVariables.size(); ++i) {
      size_t EnvpSize = EnvironmentVariables[i].size();
      // Set the pointer to this argument
      EnvpPointers[i] = EnvpBackingBaseGuest + CurrentOffset;

      // Copy the string in to the final location
      memcpy(reinterpret_cast<void*>(EnvpBackingBase + CurrentOffset), &EnvironmentVariables[i].at(0), EnvpSize);

      // Set the null terminator for the string
      *reinterpret_cast<uint8_t*>(EnvpBackingBase + CurrentOffset + EnvpSize + 1) = 0;

      CurrentOffset += EnvpSize + 1;
    }

    // Last envp needs to be nullptr
    EnvpPointers[EnvironmentVariables.size()] = 0;

    for (size_t i = 0; auto const &Variable : AuxVariables) {
      AuxVPointers[i].key = Variable.key;
      AuxVPointers[i].val = Variable.val;
      ++i;
    }

    *AuxTabBase = reinterpret_cast<uint64_t>(AuxVPointers);
    *AuxTabSize = sizeof(AuxType) * AuxVariables.size();
  }

  // Setups the stack initial data (argv, envp, auxv)
  void SetupStack() {
    StackPointer += StackSize();
    // Set up our initial CPU state
    uint64_t SizeOfPointer = Is64BitMode() ? 8 : 4;

    uint64_t TotalArgumentMemSize{};

    TotalArgumentMemSize += SizeOfPointer; // Argument counter size
    TotalArgumentMemSize += SizeOfPointer * Args.size(); // Pointers to strings
    TotalArgumentMemSize += SizeOfPointer; // Padding for something
    TotalArgumentMemSize += SizeOfPointer * EnvironmentVariables.size(); // Argument location for envp
    TotalArgumentMemSize += SizeOfPointer; // envp nullptr ender

    uint64_t AuxVOffset = TotalArgumentMemSize;
    if (SizeOfPointer == 8) {
      TotalArgumentMemSize += sizeof(auxv_t) * AuxVariables.size();
    }
    else {
      TotalArgumentMemSize += sizeof(auxv32_t) * AuxVariables.size();
    }

    uint64_t ArgumentOffset = TotalArgumentMemSize;
    TotalArgumentMemSize += ArgumentBackingSize;

    uint64_t EnvpOffset = TotalArgumentMemSize;
    TotalArgumentMemSize += EnvironmentBackingSize;

    // Random number location
    uint64_t RandomNumberLocation = TotalArgumentMemSize;
    TotalArgumentMemSize += 16;

    uint64_t PlatformNameLocation =  TotalArgumentMemSize;
    TotalArgumentMemSize += platform_string_max_size;

    // Offset the stack by how much memory we need
    StackPointer -= TotalArgumentMemSize;

    // Setup our AUXP values that need memory now that the stack is setup
    AuxPlatform->val = StackPointer + PlatformNameLocation;
    char *PlatformLoc = reinterpret_cast<char*>(AuxPlatform->val);
    memset(PlatformLoc, 0, platform_string_max_size);
    if (Is64BitMode()) {
      strncpy(PlatformLoc, platform_name_x86_64.data(), platform_string_max_size);
    }
    else {
      strncpy(PlatformLoc, platform_name_i686.data(), platform_string_max_size);
    }

    // Random value is always 128bits
    AuxRandom->val = StackPointer + RandomNumberLocation;
    uint64_t *RandomLoc = reinterpret_cast<uint64_t*>(AuxRandom->val);
    uint64_t *HostRandom = reinterpret_cast<uint64_t*>(getauxval(AT_RANDOM));
    if (HostRandom) {
      // Pass through the host's random values
      RandomLoc[0] = HostRandom[0];
      RandomLoc[1] = HostRandom[1];
    }
    else {
      // Nothing provided from the kernel, generate our own random values.
      if (!GetRandom(&RandomLoc[0], sizeof(uint64_t) * 2)) {
        // getrandom failed for some reason.
        RandomLoc[0] = 0;
        RandomLoc[1] = 0;
        LogMan::Msg::EFmt("RNG failed. AT_RANDOM will not be random.");
      }
    }

    // Stack setup
    // [0, 8):   Argument Count
    // [8, 16):  Argument Pointer 0
    // [16, 24): Argument Pointer 1
    // ....
    // [Pad1, +8): Some Pointer
    // [envp, +8): envp pointer
    // [Pad2End, +8): Argument String 0
    // [+8, +8): String 1
    // ...
    // [argvend, +8): envp[0]
    // ...
    // [envpend, +8): nullptr

    if (SizeOfPointer == 8) {
      SetupPointers<uint64_t, auxv_t, 8>(
        StackPointer,
        AuxVOffset,
        ArgumentOffset,
        EnvpOffset,
        Args,
        EnvironmentVariables,
        AuxVariables,
        &AuxTabBase,
        &AuxTabSize,
        RandomNumberLocation,
        PlatformNameLocation
        );
    }
    else {
      SetupPointers<uint32_t, auxv32_t, 4>(
        StackPointer,
        AuxVOffset,
        ArgumentOffset,
        EnvpOffset,
        Args,
        EnvironmentVariables,
        AuxVariables,
        &AuxTabBase,
        &AuxTabSize,
        RandomNumberLocation,
        PlatformNameLocation
        );
    }
  }

  std::vector<std::string> const *GetApplicationArguments() override { return &Args; }
  void GetExecveArguments(fextl::vector<char const*> *Args) override { *Args = LoaderArgs; }

  void GetAuxv(uint64_t& addr, uint64_t& size) override {
    addr = AuxTabBase;
    size = AuxTabSize;
  }

  uint64_t GetBaseOffset() const override {
    return BaseOffset;
  }

  bool Is64BitMode() const {
    return MainElf.type == ::ELFLoader::ELFContainer::TYPE_X86_64;
  }

  ::ELFLoader::ELFContainer::BRKInfo GetBRKInfo() {
    return ::ELFLoader::ELFContainer::BRKInfo { BrkStart, BRK_SIZE };
  }

  bool ELFWasLoaded() {
    return ElfValid;
  }

  void SetVDSOBase(void* Base) {
    VDSOBase = Base;
  }

  void CalculateHWCaps(FEXCore::Context::Context *ctx) {
    // HWCAP is just CPUID function 0x1, the EDX result
    auto res_1 = ctx->RunCPUIDFunction(1, 0);
    HWCap = res_1.edx;

    // HWCAP2 is as follows:
    // Bits:
    // 0 - MONITOR/MWAIT available in CPL3
    // 1 - FSGSBASE instructions available in CPL3
    HWCap2 = 0;

    // We need to know if we support AVX for AT_MINSIGSTKSZ
    SupportsAVX = !!(res_1.ecx & (1U << 28));
  }

  uint64_t CalculateSignalStackSize() const {
    // We must calculate the required signal stack size that the "kernel" consumes.
    // For FEX this means the amount of state we store in to the guest stack, not including the amount
    // that FEX stores in to the host stack as well.
    //
    // This needs to match what we do in FEXCore's dispatcher (Which should at some point be moved to the frontend).
    //
    // This roughly means that we need to calculate the combined size of:
    // - xstate or _libc_fstate depending on AVX support
    // - ucontext_t
    // - siginfo_t
    // Size of state requiring to be stored is different between 32-bit and 64-bit.

    uint64_t Result{};
    if (Is64BitMode()) {
      Result += sizeof(FEXCore::x86_64::ucontext_t);
      Result = FEXCore::AlignUp(Result, alignof(FEXCore::x86_64::ucontext_t));
      if (SupportsAVX) {
        Result += sizeof(FEXCore::x86_64::xstate);
        Result = FEXCore::AlignUp(Result, alignof(FEXCore::x86_64::xstate));
      }
      else {
        Result += sizeof(FEXCore::x86_64::_libc_fpstate);
        Result = FEXCore::AlignUp(Result, alignof(FEXCore::x86_64::_libc_fpstate));
      }

      Result += sizeof(siginfo_t);
      Result = FEXCore::AlignUp(Result, alignof(siginfo_t));
    }
    else {
      Result += sizeof(FEXCore::x86::ucontext_t);
      Result = FEXCore::AlignUp(Result, alignof(FEXCore::x86::ucontext_t));
      if (SupportsAVX) {
        Result += sizeof(FEXCore::x86::xstate);
        Result = FEXCore::AlignUp(Result, alignof(FEXCore::x86::xstate));
      }
      else {
        Result += sizeof(FEXCore::x86::_libc_fpstate);
        Result = FEXCore::AlignUp(Result, alignof(FEXCore::x86::_libc_fpstate));
      }

      Result += sizeof(FEXCore::x86::siginfo_t);
      Result = FEXCore::AlignUp(Result, alignof(FEXCore::x86::siginfo_t));
    }

    return Result;
  }

  constexpr static uint64_t BRK_SIZE = 8 * 1024 * 1024;
  constexpr static uint64_t STACK_SIZE = 8 * 1024 * 1024;
  constexpr static uint64_t FULL_STACK_SIZE = 128 * 1024 * 1024;
  constexpr static uint64_t STACK_HINT_32 = 0xFFFFE000 - FULL_STACK_SIZE;
  constexpr static uint64_t STACK_HINT_64 = 0x7FFFFFFFF000 - FULL_STACK_SIZE;

  std::vector<std::string> Args;
  std::vector<std::string> EnvironmentVariables;
  fextl::vector<char const*> LoaderArgs;

  std::list<auxv_t> AuxVariables;
  uint64_t AuxTabBase, AuxTabSize;
  uint64_t ArgumentBackingSize{};
  uint64_t EnvironmentBackingSize{};
  uint64_t BaseOffset{};
  void* VDSOBase{};
  uint64_t HWCap{};
  uint64_t HWCap2{};
  bool SupportsAVX{};

  auxv_t *AuxRandom{};
  auxv_t *AuxPlatform{};

  static constexpr std::string_view platform_name_x86_64 = "x86_64";
  static constexpr std::string_view platform_name_i686 = "i686";
  // Need to include null character.
  static constexpr size_t platform_string_max_size = std::max(platform_name_x86_64.size(), platform_name_i686.size()) + 1;

  FEX_CONFIG_OPT(AdditionalArguments, ADDITIONALARGUMENTS);
  FEX_CONFIG_OPT(InjectLibSegFault, INJECTLIBSEGFAULT);

};
