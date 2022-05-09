#pragma once

#include "Common/Config.h"
#include "Linux/Utils/ELFContainer.h"
#include "Linux/Utils/ELFSymbolDatabase.h"

#include <array>
#include <bitset>
#include <cassert>
#include <cstring>
#include <fstream>
#include <sys/mman.h>
#include <vector>

#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>

namespace FEX::HarnessHelper {

class ELFCodeLoader final : public FEXCore::CodeLoader {
private:
  struct auxv_t {
    uint64_t key;
    uint64_t val;
  };

public:
  ELFCodeLoader(std::string const &Filename, std::string const &RootFS, [[maybe_unused]] std::vector<std::string> const &args, std::vector<std::string> const &ParsedArgs, char **const envp = nullptr, FEXCore::Config::Value<std::string> *AdditionalEnvp = nullptr)
    : File {Filename, RootFS, false}
    , DB {&File}
    , Args {args} {

/*
    if (File.HasDynamicLinker()) {
      // If the file isn't static then we need to add the filename of interpreter
      // to the front of the argument list
      Args.emplace(Args.begin(), File.InterpreterLocation());
    }*/

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

    AuxVariables.emplace_back(auxv_t{11, 1000}); // AT_UID
    AuxVariables.emplace_back(auxv_t{12, 1000}); // AT_EUID
    AuxVariables.emplace_back(auxv_t{13, 1000}); // AT_GID
    AuxVariables.emplace_back(auxv_t{14, 1000}); // AT_EGID
    AuxVariables.emplace_back(auxv_t{17, 0x64}); // AT_CLKTIK
    AuxVariables.emplace_back(auxv_t{6, 0x1000}); // AT_PAGESIZE
    AuxVariables.emplace_back(auxv_t{25, ~0ULL}); // AT_RANDOM
    AuxVariables.emplace_back(auxv_t{23, 0}); // AT_SECURE
    AuxVariables.emplace_back(auxv_t{8, 0}); // AT_FLAGS
    AuxVariables.emplace_back(auxv_t{5, File.GetProgramHeaderCount()});

    if (File.GetMode() == ELFLoader::ELFContainer::MODE_64BIT) {
      AuxVariables.emplace_back(auxv_t{4, 0x38}); // AT_PHENT
      // On x86 this is the value returned from CPUID 01h EDX
      AuxVariables.emplace_back(auxv_t{16, 0}); // AT_HWCAP

      //AuxVariables.emplace_back(auxv_t{24, ~0ULL}); // AT_PLATFORM
      // On x86 only allows userspace to check for monitor and fs/gs base writing in CPL3
      //AuxVariables.emplace_back(auxv_t{26, 0}); // AT_HWCAP2
      AuxVariables.emplace_back(auxv_t{32, 0}); // AT_SYSINFO - Entry point to syscall
      AuxVariables.emplace_back(auxv_t{33, 0}); // AT_SYSINFO_EHDR - Address of the start of VDSO
    }
    else {
      AuxVariables.emplace_back(auxv_t{4, 0x20}); // AT_PHENT
      AuxVariables.emplace_back(auxv_t{32, 0}); // AT_SYSINFO - Entry point to syscall
      AuxVariables.emplace_back(auxv_t{33, 0}); // AT_SYSINFO_EHDR - Address of the start of VDSO
    }

    AuxVariables.emplace_back(auxv_t{3, DB.GetElfBase()}); // Program header
    AuxVariables.emplace_back(auxv_t{7, DB.GetElfBase()}); // Interpreter address
    AuxVariables.emplace_back(auxv_t{9, DB.DefaultRIP()}); // AT_ENTRY
    AuxVariables.emplace_back(auxv_t{0, 0}); // Null ender

    for (auto &Arg : ParsedArgs) {
      LoaderArgs.emplace_back(Arg.c_str());
    }
  }

  uint64_t StackSize() const override {
    return STACK_SIZE;
  }

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

  uint64_t GetStackPointer() override {
    uintptr_t StackPointer{};
    StackPointer = reinterpret_cast<uintptr_t>(FEXCore::Allocator::mmap(nullptr, StackSize(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

    StackPointer += StackSize();
    // Set up our initial CPU state
    uint64_t SizeOfPointer = File.GetMode() == ELFLoader::ELFContainer::MODE_64BIT ? 8 : 4;

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

    return StackPointer;
  }

  uint64_t DefaultRIP() const override {
    return DB.DefaultRIP();
  }

  bool MapMemory(const MapperFn& Mapper, const UnmapperFn& Unmapper) override {
    auto DoMMap = [&Mapper](uint64_t Address, size_t Size, bool FixedNoReplace) -> void* {
      void *Result = Mapper(reinterpret_cast<void*>(Address), Size, PROT_READ | PROT_WRITE, (FixedNoReplace ? MAP_FIXED_NOREPLACE : MAP_FIXED) | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      LOGMAN_THROW_A_FMT(Result != (void*)~0ULL, "Couldn't mmap");
      return Result;
    };

    DB.MapMemoryRegions(DoMMap);

    LoadMemory();

    return true;
  }

  void LoadMemory() {
    auto ELFLoaderWrapper = [&](void const *Data, uint64_t Addr, uint64_t Size) -> void {
      memcpy(reinterpret_cast<void*>(Addr), Data, Size);
    };
    DB.WriteLoadableSections(ELFLoaderWrapper);
  }

  char const *FindSymbolNameInRange(uint64_t Address) {

    ELFLoader::ELFSymbol const *Sym;
    Sym = DB.GetSymbolInRange(std::make_pair(Address, 1));
    if (Sym) {
      return Sym->Name;
    }

    return nullptr;
  }

  void GetInitLocations(std::vector<uint64_t> *Locations) {
    DB.GetInitLocations(Locations);
  }

  std::vector<std::string> const *GetApplicationArguments() override { return &Args; }
  void GetExecveArguments(std::vector<char const*> *Args) override { *Args = LoaderArgs; }

  void GetAuxv(uint64_t& addr, uint64_t& size) override {
    addr = AuxTabBase;
    size = AuxTabSize;
  }

  bool Is64BitMode() const { return File.GetMode() == ::ELFLoader::ELFContainer::MODE_64BIT; }

  ::ELFLoader::ELFContainer::BRKInfo GetBRKInfo() const {
    auto Info = File.GetBRKInfo();
    Info.Base += DB.GetElfBase();
    return Info;
  }

  bool ELFWasLoaded() const { return File.WasLoaded(); }

private:
  ::ELFLoader::ELFContainer File;
  ::ELFLoader::ELFSymbolDatabase DB;

  std::vector<std::string> Args;
  std::vector<std::string> EnvironmentVariables;
  std::vector<char const*> LoaderArgs;
  struct auxv32_t {
    uint32_t key;
    uint32_t val;
  };
  std::vector<auxv_t> AuxVariables;
  uint64_t AuxTabBase, AuxTabSize;
  uint64_t ArgumentBackingSize{};
  uint64_t EnvironmentBackingSize{};

  constexpr static uint64_t STACK_SIZE = 8 * 1024 * 1024;
};

}
