
#pragma once

#include "Common/Config.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Linux/Utils/ELFParser.h"
#include "Linux/Utils/ELFSymbolDatabase.h"

#include <array>
#include <bitset>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <elf.h>
#include <fcntl.h>
#include <fmt/format.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <sys/personality.h>

#define PAGE_START(x) ((x) & ~(uintptr_t)(4095))
#define PAGE_OFFSET(x) ((x) & 4095)
#define PAGE_ALIGN(x) (((x) + 4095) & ~(uintptr_t)(4095))

class ELFCodeLoader2 final : public FEXCore::CodeLoader {
  public:
    struct LoadedSection {
      uintptr_t ElfBase;
      uintptr_t Base;
      size_t Size;
      off_t Offs;
      std::string Filename;
      bool Executable;
    };

    std::vector<LoadedSection> Sections;
  private:

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


  static std::string get_fdpath(int fd)
  {
    std::error_code ec;
    return std::filesystem::canonical(std::filesystem::path("/proc/self/fd") / std::to_string(fd), ec).string();
  }

  size_t CalculateTotalElfSize(const std::vector<Elf64_Phdr> &headers)
  {
    auto first = std::find_if(headers.begin(), headers.end(), [](const Elf64_Phdr &Header) { return Header.p_type == PT_LOAD; });
    auto last = std::find_if(headers.rbegin(), headers.rend(), [](const Elf64_Phdr &Header) { return Header.p_type == PT_LOAD; });

    if (first == headers.end())
      return 0;

    return PAGE_ALIGN(last->p_vaddr + last->p_memsz);
  }

  template<typename T>
  bool MapFile(const ELFParser& file, uintptr_t Base, const Elf64_Phdr &Header, int prot, int flags, T Mapper, std::vector<LoadedSection> *SectionArray) {

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
      auto Filename = get_fdpath(file.fd);
      if ((prot & PROT_EXEC) != 0) {
        // Push executable sections to the front
        SectionArray->insert(SectionArray->begin(), {Base, (uintptr_t)rv, size, (off_t)off, Filename, (prot & PROT_EXEC) != 0});
      }
      else {
        SectionArray->push_back({Base, (uintptr_t)rv, size, (off_t)off, Filename, (prot & PROT_EXEC) != 0});
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
  std::optional<uintptr_t> LoadElfFile(ELFParser& Elf, uintptr_t *BrkBase, TMap Mapper, TUnmap Unmapper, std::vector<LoadedSection> *SectionArray) {

    uintptr_t LoadBase = 0;

    if (BrkBase) {
      *BrkBase = 0;
    }

    if (Elf.ehdr.e_type == ET_DYN) {
      // needs base address
      auto TotalSize = CalculateTotalElfSize(Elf.phdrs) + (BrkBase ? BRK_SIZE : 0);
      LoadBase = (uintptr_t)Mapper(0, TotalSize, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
      if ((void*)LoadBase == MAP_FAILED) {
        return {};
      }

      if (Unmapper((void*)LoadBase, TotalSize) == -1) {
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
      int MapType = MAP_PRIVATE | MAP_DENYWRITE | MAP_FIXED_NOREPLACE;

			if (!MapFile(Elf, LoadBase, Header, MapProt, MapType, Mapper, SectionArray)) {
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

  public:

  static std::string ResolveRootfsFile(std::string const &File, std::string RootFS) {
    // If the path is relative then just run that
    if (std::filesystem::path(File).is_relative()) {
      return File;
    }

    std::string RootFSLink = RootFS + File;

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

    return RootFSLink;
  }

  ELFCodeLoader2(std::string const &Filename, std::string const &RootFS, [[maybe_unused]] std::vector<std::string> const &args, std::vector<std::string> const &ParsedArgs, char **const envp = nullptr, FEXCore::Config::Value<std::string> *AdditionalEnvp = nullptr) :
    Args {args} {

    bool LoadedWithFD = false;
    int FD = getauxval(AT_EXECFD);

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
    StackPointer = reinterpret_cast<uintptr_t>(Mapper(nullptr, StackSize(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK | MAP_GROWSDOWN, -1, 0));

    if (StackPointer == ~0ULL) {
      LogMan::Msg::EFmt("Allocating stack failed");
      return false;
    }

    // load the main elf
    std::vector<LoadedSection> ELFSections;
    std::vector<LoadedSection> InterpreterSections;

    uintptr_t BrkBase = 0;

    uintptr_t LoadBase = 0;

    if (auto elf = LoadElfFile(MainElf, &BrkBase, Mapper, Unmapper, &ELFSections)) {
      LoadBase = *elf;
      if (MainElf.ehdr.e_type == ET_DYN) {
        BaseOffset = LoadBase;
      }
    } else {
      LogMan::Msg::EFmt("Failed to load elf file");
      return false;
    }

    // XXX Randomise brk?

    BrkStart = (uint64_t)Mapper((void*)BrkBase, BRK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED_NOREPLACE, -1, 0);

    if ((void*)BrkStart == MAP_FAILED) {
      LogMan::Msg::EFmt("Failed to allocate BRK @ {:x}, {}\n", BrkBase, errno);
      return false;
    }

    MainElfBase = LoadBase + MainElf.phdrs.front().p_vaddr - MainElf.phdrs.front().p_offset;
    MainElfEntrypoint = LoadBase + MainElf.ehdr.e_entry;

    if (!MainElf.InterpreterElf.empty()) {
      uint64_t InterpLoadBase = 0;
      if (auto elf = LoadElfFile(InterpElf, nullptr, Mapper, Unmapper, &InterpreterSections)) {
        InterpLoadBase = *elf;
      } else {
        LogMan::Msg::EFmt("Failed to load interpreter elf file");
        return false;
      }

      InterpeterElfBase = InterpLoadBase + InterpElf.phdrs.front().p_vaddr - InterpElf.phdrs.front().p_offset;
      Entrypoint = InterpLoadBase + InterpElf.ehdr.e_entry;
    } else {
      InterpeterElfBase = 0;
      Entrypoint = MainElfEntrypoint;
    }

    // Add the interpreter sections first then the main ELF sections
    // This ensures that our code caching loads interpreter first, which is where code entry will be
    Sections.insert(Sections.end(), InterpreterSections.begin(), InterpreterSections.end());
    Sections.insert(Sections.end(), ELFSections.begin(), ELFSections.end());

    // All done

    // Setup AuxVars
    AuxVariables.emplace_back(auxv_t{11, getauxval(AT_UID)}); // AT_UID
    AuxVariables.emplace_back(auxv_t{12, getauxval(AT_EUID)}); // AT_EUID
    AuxVariables.emplace_back(auxv_t{13, getauxval(AT_GID)}); // AT_GID
    AuxVariables.emplace_back(auxv_t{14, getauxval(AT_EGID)}); // AT_EGID
    AuxVariables.emplace_back(auxv_t{17, getauxval(AT_CLKTCK)}); // AT_CLKTIK
    AuxVariables.emplace_back(auxv_t{6, 0x1000}); // AT_PAGESIZE
    AuxVariables.emplace_back(auxv_t{25, ~0ULL}); // AT_RANDOM
    AuxVariables.emplace_back(auxv_t{23, 0}); // AT_SECURE
    AuxVariables.emplace_back(auxv_t{8, 0}); // AT_FLAGS
    AuxVariables.emplace_back(auxv_t{5, MainElf.phdrs.size()}); // AT_PHNUM

    if (Is64BitMode()) {
      AuxVariables.emplace_back(auxv_t{4, 0x38}); // AT_PHENT
      // On x86 this is the value returned from CPUID 01h EDX
      AuxVariables.emplace_back(auxv_t{16, 0}); // AT_HWCAP

      //AuxVariables.emplace_back(auxv_t{24, ~0ULL}); // AT_PLATFORM
      // On x86 only allows userspace to check for monitor and fs/gs base writing in CPL3
      //AuxVariables.emplace_back(auxv_t{26, 0}); // AT_HWCAP2

      // we don't support vsyscall or vDSO so we don't set those
      //AuxVariables.emplace_back(auxv_t{32, 0}); // AT_SYSINFO - Entry point to syscall
      //AuxVariables.emplace_back(auxv_t{33, 0}); // AT_SYSINFO_EHDR - Address of the start of VDSO
    }
    else {
      AuxVariables.emplace_back(auxv_t{4, 0x20}); // AT_PHENT

      // we don't support vsyscall or vDSO so we don't set those
      //AuxVariables.emplace_back(auxv_t{32, 0}); // AT_SYSINFO - Entry point to syscall
      //AuxVariables.emplace_back(auxv_t{33, 0}); // AT_SYSINFO_EHDR - Address of the start of VDSO
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
    const std::vector<auxv_t> &AuxVariables,
    uint64_t *AuxTabBase,
    uint64_t *AuxTabSize,
    PointerType RandomNumberOffset
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

    for (size_t i = 0; i < AuxVariables.size(); ++i) {
      if (AuxVariables[i].key == 25) {
        // Random value is always 128bits
        AuxType Random{25, static_cast<PointerType>(StackPointer + RandomNumberOffset)};
        uint64_t *RandomLoc = reinterpret_cast<uint64_t*>(StackPointer + RandomNumberOffset);
        RandomLoc[0] = 0xDEAD;
        RandomLoc[1] = 0xDEAD2;
        AuxVPointers[i].key = Random.key;
        AuxVPointers[i].val = Random.val;
      }
      else {
        AuxVPointers[i].key = AuxVariables[i].key;
        AuxVPointers[i].val = AuxVariables[i].val;
      }
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
    uint32_t RandomNumberLocation = TotalArgumentMemSize;
    TotalArgumentMemSize += 16;

    // Offset the stack by how much memory we need
    StackPointer -= TotalArgumentMemSize;

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
        RandomNumberLocation
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
        RandomNumberLocation
        );
    }
  }

  std::vector<std::string> const *GetApplicationArguments() override { return &Args; }
  void GetExecveArguments(std::vector<char const*> *Args) override { *Args = LoaderArgs; }

  void GetAuxv(uint64_t& addr, uint64_t& size) override {
    addr = AuxTabBase;
    size = AuxTabSize;
  }

  uint64_t GetBaseOffset() const override {
    return BaseOffset;
  }

  bool Is64BitMode() {
    return MainElf.type == ::ELFLoader::ELFContainer::TYPE_X86_64;
  }

  ::ELFLoader::ELFContainer::BRKInfo GetBRKInfo() {
    return ::ELFLoader::ELFContainer::BRKInfo { BrkStart, BRK_SIZE };
  }

  bool ELFWasLoaded() {
    return ElfValid;
  }

  constexpr static uint64_t BRK_SIZE = 8 * 1024 * 1024;
  constexpr static uint64_t STACK_SIZE = 8 * 1024 * 1024;

  std::vector<std::string> Args;
  std::vector<std::string> EnvironmentVariables;
  std::vector<char const*> LoaderArgs;

  std::vector<auxv_t> AuxVariables;
  uint64_t AuxTabBase, AuxTabSize;
  uint64_t ArgumentBackingSize{};
  uint64_t EnvironmentBackingSize{};
  uint64_t BaseOffset{};

};
