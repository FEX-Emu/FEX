#pragma once
#include "Common/Config.h"
#include "Common/MathUtils.h"
#include "ELFLoader.h"
#include "ELFSymbolDatabase.h"
#include "LogManager.h"

#include <FEXCore/Core/CodeLoader.h>
#include <bitset>
#include <cassert>
#include <cstring>
#include <fstream>
#include <sys/mman.h>
#include <vector>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>

namespace FEX::HarnessHelper {
  inline bool CompareStates(FEXCore::Core::CPUState const& State1,
       FEXCore::Core::CPUState const& State2,
       uint64_t MatchMask,
       bool OutputGPRs) {
    bool Matches = true;

    auto DumpGPRs = [OutputGPRs](auto Name, uint64_t A, uint64_t B) {
      if (!OutputGPRs) return;
      if (A == B) return;

      printf("%s: 0x%016lx %s 0x%016lx\n", Name.c_str(), A, A==B ? "==" : "!=", B);
    };

    auto DumpFLAGs = [OutputGPRs](auto Name, uint64_t A, uint64_t B) {
      if (!OutputGPRs) return;
      if (A == B) return;

      constexpr std::array<unsigned, 17> Flags = {
        FEXCore::X86State::RFLAG_CF_LOC,
        FEXCore::X86State::RFLAG_PF_LOC,
        FEXCore::X86State::RFLAG_AF_LOC,
        FEXCore::X86State::RFLAG_ZF_LOC,
        FEXCore::X86State::RFLAG_SF_LOC,
        FEXCore::X86State::RFLAG_TF_LOC,
        FEXCore::X86State::RFLAG_IF_LOC,
        FEXCore::X86State::RFLAG_DF_LOC,
        FEXCore::X86State::RFLAG_OF_LOC,
        FEXCore::X86State::RFLAG_IOPL_LOC,
        FEXCore::X86State::RFLAG_NT_LOC,
        FEXCore::X86State::RFLAG_RF_LOC,
        FEXCore::X86State::RFLAG_VM_LOC,
        FEXCore::X86State::RFLAG_AC_LOC,
        FEXCore::X86State::RFLAG_VIF_LOC,
        FEXCore::X86State::RFLAG_VIP_LOC,
        FEXCore::X86State::RFLAG_ID_LOC,
      };

      printf("%s: 0x%016lx %s 0x%016lx\n", Name.c_str(), A, A==B ? "==" : "!=", B);
      for (auto &Flag : Flags) {
        uint64_t FlagMask = 1 << Flag;
        if ((A & FlagMask) != (B & FlagMask)) {
          printf("\t%s: %ld != %ld\n", FEXCore::Core::GetFlagName(Flag).data(), (A >> Flag) & 1, (B >> Flag) & 1);
        }
      }
    };

    auto CheckGPRs = [&Matches, DumpGPRs](std::string Name, uint64_t A, uint64_t B){
      DumpGPRs(std::move(Name), A, B);
      Matches &= A == B;
    };

    auto CheckFLAGS = [&Matches, DumpFLAGs](std::string Name, uint64_t A, uint64_t B){
      DumpFLAGs(std::move(Name), A, B);
      Matches &= A == B;
    };


    // RIP
    if (MatchMask & 1) {
      CheckGPRs("RIP", State1.rip, State2.rip);
    }

    MatchMask >>= 1;

    // GPRS
    for (unsigned i = 0; i < 16; ++i, MatchMask >>= 1) {
      if (MatchMask & 1) {
        CheckGPRs("GPR" + std::to_string(i), State1.gregs[i], State2.gregs[i]);
      }
    }

    // XMM
    for (unsigned i = 0; i < 16; ++i, MatchMask >>= 1) {
      if (MatchMask & 1) {
        CheckGPRs("XMM0_" + std::to_string(i), State1.xmm[i][0], State2.xmm[i][0]);
        CheckGPRs("XMM1_" + std::to_string(i), State1.xmm[i][1], State2.xmm[i][1]);
      }
    }

    // GS
    if (MatchMask & 1) {
      CheckGPRs("GS", State1.gs, State2.gs);
    }
    MatchMask >>= 1;

    // FS
    if (MatchMask & 1) {
      CheckGPRs("FS", State1.fs, State2.fs);
    }
    MatchMask >>= 1;

    auto CompactRFlags = [](auto Arg) -> uint32_t {
      uint32_t Res = 2;
      for (int i = 0; i < 32; ++i) {
        Res |= Arg->flags[i] << i;
      }
      return Res;
    };

    // FLAGS
    if (MatchMask & 1) {
      uint32_t rflags1 = CompactRFlags(&State1);
      uint32_t rflags2 = CompactRFlags(&State2);

      CheckFLAGS("FLAGS", rflags1, rflags2);
    }
    MatchMask >>= 1;
    return Matches;
  }

  inline void ReadFile(std::string const &Filename, std::vector<char> *Data) {
    std::fstream TestFile;
    TestFile.open(Filename, std::fstream::in | std::fstream::binary);
    LogMan::Throw::A(TestFile.is_open(), "Failed to open file");

    TestFile.seekg(0, std::fstream::end);
    size_t FileSize = TestFile.tellg();
    TestFile.seekg(0, std::fstream::beg);

    Data->resize(FileSize);

    TestFile.read(&Data->at(0), FileSize);

    TestFile.close();
  }

  class ConfigLoader final {
  public:
    void Init(std::string const &ConfigFilename) {
      ReadFile(ConfigFilename, &RawConfigFile);
      memcpy(&BaseConfig, RawConfigFile.data(), sizeof(ConfigStructBase));
    }

    bool CompareStates(FEXCore::Core::CPUState const* State1, FEXCore::Core::CPUState const* State2) {
      bool Matches = true;
      uint64_t MatchMask = BaseConfig.OptionMatch & ~BaseConfig.OptionIgnore;
      if (State1 && State2) {
        Matches &= FEX::HarnessHelper::CompareStates(*State1, *State2, MatchMask, ConfigDumpGPRs());
      }

      if (BaseConfig.OptionRegDataCount > 0) {
        constexpr std::array<uint64_t, 45> OffsetArray = {{
          offsetof(FEXCore::Core::CPUState, rip),
          offsetof(FEXCore::Core::CPUState, gregs[0]),
          offsetof(FEXCore::Core::CPUState, gregs[1]),
          offsetof(FEXCore::Core::CPUState, gregs[2]),
          offsetof(FEXCore::Core::CPUState, gregs[3]),
          offsetof(FEXCore::Core::CPUState, gregs[4]),
          offsetof(FEXCore::Core::CPUState, gregs[5]),
          offsetof(FEXCore::Core::CPUState, gregs[6]),
          offsetof(FEXCore::Core::CPUState, gregs[7]),
          offsetof(FEXCore::Core::CPUState, gregs[8]),
          offsetof(FEXCore::Core::CPUState, gregs[9]),
          offsetof(FEXCore::Core::CPUState, gregs[10]),
          offsetof(FEXCore::Core::CPUState, gregs[11]),
          offsetof(FEXCore::Core::CPUState, gregs[12]),
          offsetof(FEXCore::Core::CPUState, gregs[13]),
          offsetof(FEXCore::Core::CPUState, gregs[14]),
          offsetof(FEXCore::Core::CPUState, gregs[15]),
          offsetof(FEXCore::Core::CPUState, xmm[0][0]),
          offsetof(FEXCore::Core::CPUState, xmm[1][0]),
          offsetof(FEXCore::Core::CPUState, xmm[2][0]),
          offsetof(FEXCore::Core::CPUState, xmm[3][0]),
          offsetof(FEXCore::Core::CPUState, xmm[4][0]),
          offsetof(FEXCore::Core::CPUState, xmm[5][0]),
          offsetof(FEXCore::Core::CPUState, xmm[6][0]),
          offsetof(FEXCore::Core::CPUState, xmm[7][0]),
          offsetof(FEXCore::Core::CPUState, xmm[8][0]),
          offsetof(FEXCore::Core::CPUState, xmm[9][0]),
          offsetof(FEXCore::Core::CPUState, xmm[10][0]),
          offsetof(FEXCore::Core::CPUState, xmm[11][0]),
          offsetof(FEXCore::Core::CPUState, xmm[12][0]),
          offsetof(FEXCore::Core::CPUState, xmm[13][0]),
          offsetof(FEXCore::Core::CPUState, xmm[14][0]),
          offsetof(FEXCore::Core::CPUState, xmm[15][0]),
          offsetof(FEXCore::Core::CPUState, gs),
          offsetof(FEXCore::Core::CPUState, fs),
          offsetof(FEXCore::Core::CPUState, flags),
          offsetof(FEXCore::Core::CPUState, mm[0][0]),
          offsetof(FEXCore::Core::CPUState, mm[1][0]),
          offsetof(FEXCore::Core::CPUState, mm[2][0]),
          offsetof(FEXCore::Core::CPUState, mm[3][0]),
          offsetof(FEXCore::Core::CPUState, mm[4][0]),
          offsetof(FEXCore::Core::CPUState, mm[5][0]),
          offsetof(FEXCore::Core::CPUState, mm[6][0]),
          offsetof(FEXCore::Core::CPUState, mm[7][0]),
          offsetof(FEXCore::Core::CPUState, mm[8][0]),
        }};

        uintptr_t DataOffset = BaseConfig.OptionRegDataOffset;
        for (unsigned i = 0; i < BaseConfig.OptionRegDataCount; ++i) {
          RegDataStructBase *RegData = reinterpret_cast<RegDataStructBase*>(RawConfigFile.data() + DataOffset);
          [[maybe_unused]] std::bitset<64> RegFlags = RegData->RegKey;
          assert(RegFlags.count() == 1  && "Must set reg data explicitly per register");

          size_t NameIndex = __builtin_ffsl(RegData->RegKey)- 1;
          auto Offset = OffsetArray[NameIndex];
          uint64_t *State1Data = reinterpret_cast<uint64_t*>(reinterpret_cast<uint64_t>(State1) + Offset);
          uint64_t *State2Data = reinterpret_cast<uint64_t*>(reinterpret_cast<uint64_t>(State2) + Offset);

          auto DumpGPRs = [this](auto Name, uint64_t A, uint64_t B) {
            if (!ConfigDumpGPRs())
              return;

            printf("%s: 0x%016lx %s 0x%016lx (Expected)\n", Name.c_str(), A, A==B ? "==" : "!=", B);
          };

          auto CheckGPRs = [&Matches, DumpGPRs](std::string Name, uint64_t A, uint64_t B) {
            DumpGPRs(std::move(Name), A, B);
            Matches &= A == B;
          };

          for (unsigned j = 0; j < RegData->RegDataCount; ++j) {
            std::string Name;
            if (NameIndex == 0) // RIP
              Name = "RIP";
            else if (NameIndex >= 1 && NameIndex < 17)
              Name = "GPR" + std::to_string(NameIndex - 1);
            else if (NameIndex >= 17 && NameIndex < 33)
              Name = "XMM[" + std::to_string(NameIndex - 17) + "][" + std::to_string(j) + "]";
            else if (NameIndex == 33)
              Name = "gs";
            else if (NameIndex == 34)
              Name ="fs";
            else if (NameIndex == 35)
              Name = "rflags";
            else if (NameIndex >= 36 && NameIndex < 45)
              Name = "MM[" + std::to_string(NameIndex - 36) + "][" + std::to_string(j) + "]";


            if (State1) {
              CheckGPRs("Core1: " + Name + ": ", State1Data[j], RegData->RegValues[j]);
            }
            if (State2) {
              CheckGPRs("Core2: " + Name + ": ", State2Data[j], RegData->RegValues[j]);
            }
          }

          // Get the correct data offset
          DataOffset += sizeof(RegDataStructBase) + RegData->RegDataCount * 8;
        }
      }
      return Matches;
    }

    std::map<uintptr_t, size_t> GetMemoryRegions() {
      std::map<uintptr_t, size_t> regions;

      uintptr_t DataOffset = BaseConfig.OptionMemoryRegionOffset;
      for (unsigned i = 0; i < BaseConfig.OptionMemoryRegionCount; ++i) {
        MemoryRegionBase *Region = reinterpret_cast<MemoryRegionBase*>(RawConfigFile.data() + DataOffset);
        regions[Region->Region] = Region->Size;

        DataOffset += sizeof(MemoryRegionBase);
      }

      return regions;
    }

    void LoadMemory(uint64_t MemoryBase, FEXCore::CodeLoader::MemoryWriter Writer) {
      uintptr_t DataOffset = BaseConfig.OptionMemDataOffset;
      for (unsigned i = 0; i < BaseConfig.OptionMemDataCount; ++i) {
        MemDataStructBase *MemData = reinterpret_cast<MemDataStructBase*>(RawConfigFile.data() + DataOffset);

        Writer(&MemData->data, MemoryBase + MemData->address, MemData->length);

        DataOffset += sizeof(MemDataStructBase) + MemData->length;
      }
    }

    bool Is64BitMode() const { return BaseConfig.OptionMode == 1; }

  private:
    FEX::Config::Value<bool> ConfigDumpGPRs{"DumpGPRs", false};

    struct ConfigStructBase {
      uint64_t OptionMatch;
      uint64_t OptionIgnore;
      uint64_t OptionStackSize;
      uint64_t OptionEntryPoint;
      uint32_t OptionABI;
      uint32_t OptionMode;
      uint32_t OptionMemoryRegionOffset;
      uint32_t OptionMemoryRegionCount;
      uint32_t OptionRegDataOffset;
      uint32_t OptionRegDataCount;
      uint32_t OptionMemDataOffset;
      uint32_t OptionMemDataCount;
      uint8_t  AdditionalData[];
    }__attribute__((packed));

    struct MemoryRegionBase {
      uint64_t Region;
      uint64_t Size;
    } __attribute__((packed));

    struct RegDataStructBase {
      uint32_t RegDataCount;
      uint64_t RegKey;
      uint64_t RegValues[];
    } __attribute__((packed));

    struct MemDataStructBase {
      uint64_t address;
      uint32_t length;
      uint8_t data[];
    } __attribute__((packed));

    std::vector<char> RawConfigFile;
    ConfigStructBase BaseConfig;
  };

  class HarnessCodeLoader final : public FEXCore::CodeLoader {

    static constexpr uint32_t PAGE_SIZE = 4096;
  public:

    HarnessCodeLoader(std::string const &Filename, const char *ConfigFilename) {
      ReadFile(Filename, &RawFile);
      if (ConfigFilename) {
        Config.Init(ConfigFilename);
      }
    }

    uint64_t StackSize() const override {
      return STACK_SIZE;
    }

    void SetMemoryBase(uint64_t Base, bool Unified) override {
      MemoryBase = Base;
    }

    uint64_t SetupStack([[maybe_unused]] void *HostPtr, uint64_t GuestPtr) const override {
      return GuestPtr + STACK_SIZE - 16;
    }

    uint64_t DefaultRIP() const override {
      return RIP;
    }

    void MapMemoryRegion(std::function<void*(uint64_t, uint64_t, bool, bool)> Mapper) override {
      bool LimitedSize = true;
      if (LimitedSize) {
        Mapper(0xe000'0000, PAGE_SIZE * 10, true, false);

        // SIB8
        // We test [-128, -126] (Bottom)
        // We test [-8, 8] (Middle)
        // We test [120, 127] (Top)
        // Can fit in two pages
        Mapper(0xe800'0000 - PAGE_SIZE, PAGE_SIZE * 2, true, false);

        // SIB32 Bottom
        // We test INT_MIN, INT_MIN + 8
        Mapper(0x2'0000'0000, PAGE_SIZE, true, false);
        // SIB32 Middle
        // We test -8 + 8
        Mapper(0x2'8000'0000 - PAGE_SIZE, PAGE_SIZE * 2, true, false);

        // SIB32 Top
        // We Test INT_MAX - 8, INT_MAX
        Mapper(0x3'0000'0000 - PAGE_SIZE, PAGE_SIZE * 2, true, false);
      }
      else {
        // This is scratch memory location and SIB8 location
        Mapper(0xe000'0000, 0x1000'0000, true, false);
        // This is for large SIB 32bit displacement testing
        Mapper(0x2'0000'0000, 0x1'0000'1000, true, false);
      }

      // Map in the memory region for the test file
      size_t Length = AlignUp(RawFile.size(), PAGE_SIZE);
      Code_start_page = reinterpret_cast<uint64_t>(Mapper(Code_start_page, Length, true, true));
      mprotect(reinterpret_cast<void*>(Code_start_page), Length, PROT_READ | PROT_WRITE | PROT_EXEC);
      RIP = Code_start_page;

      // Map the memory regions the test file asks for
      for (auto& [region, size] : Config.GetMemoryRegions()) {
        Mapper(region, size, true, false);
      }
    }

    void LoadMemory(MemoryWriter Writer) override {
      // Memory base here starts at the start location we passed back with GetLayout()
      // This will write at [CODE_START_RANGE + 0, RawFile.size() )
      Writer(&RawFile.at(0), RIP, RawFile.size());

      Config.LoadMemory(0, Writer);
    }

    uint64_t GetFinalRIP() override { return RIP + RawFile.size(); }

    bool CompareStates(FEXCore::Core::CPUState const* State1, FEXCore::Core::CPUState const* State2) {
      return Config.CompareStates(State1, State2);
    }

    bool Is64BitMode() const { return Config.Is64BitMode(); }

  private:
    constexpr static uint64_t STACK_SIZE = PAGE_SIZE;
    // Zero is special case to know when we are done
    uint64_t Code_start_page = 0x0'1000;
    uint64_t RIP {};
    uint64_t MemoryBase{};

    std::vector<char> RawFile;
    ConfigLoader Config;
  };

class ELFCodeLoader final : public FEXCore::CodeLoader {
private:
  struct auxv_t {
    uint64_t key;
    uint64_t val;
  };

public:
  ELFCodeLoader(std::string const &Filename, std::string const &RootFS, [[maybe_unused]] std::vector<std::string> const &args, std::vector<std::string> const &ParsedArgs, char **const envp = nullptr, FEX::Config::Value<std::string> *AdditionalEnvp = nullptr)
    : File {Filename, RootFS, false}
    , DB {&File}
    , Args {args} {

    if (File.HasDynamicLinker()) {
      // If the file isn't static then we need to add the filename of interpreter
      // to the front of the argument list
      Args.emplace(Args.begin(), File.InterpreterLocation());
    }

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

    AuxVariables.emplace_back(auxv_t{4, 0x38}); // AT_PHENT
    AuxVariables.emplace_back(auxv_t{5, 0xb}); // XXX: AT_PHNUM
    AuxVariables.emplace_back(auxv_t{6, 0x1000}); // AT_PAGESIZE
    AuxVariables.emplace_back(auxv_t{8, 0}); // AT_FLAGS
    AuxVariables.emplace_back(auxv_t{11, 1000}); // AT_UID
    AuxVariables.emplace_back(auxv_t{12, 1000}); // AT_EUID
    AuxVariables.emplace_back(auxv_t{13, 1000}); // AT_GID
    AuxVariables.emplace_back(auxv_t{14, 1000}); // AT_EGID
    AuxVariables.emplace_back(auxv_t{16, 0}); // AT_HWCAP
    AuxVariables.emplace_back(auxv_t{17, 0x64}); // AT_CLKTIK
    AuxVariables.emplace_back(auxv_t{23, 0}); // AT_SECURE

    //AuxVariables.emplace_back(auxv_t{24, ~0ULL}); // AT_PLATFORM
    AuxVariables.emplace_back(auxv_t{25, ~0ULL}); // AT_RANDOM
    //AuxVariables.emplace_back(auxv_t{26, 0}); // AT_HWCAP2
    AuxVariables.emplace_back(auxv_t{32, 0ULL}); // sysinfo (vDSO)
    AuxVariables.emplace_back(auxv_t{33, 0ULL}); // sysinfo (vDSO)

    for (auto &Arg : ParsedArgs) {
      LoaderArgs.emplace_back(Arg.c_str());
    }
  }

  uint64_t StackSize() const override {
    return STACK_SIZE;
  }

  virtual void SetMemoryBase(uint64_t Base, bool Unified) override {
    if (File.WasDynamic() && Unified) {
      MemoryBase = Base;
    }
    else {
      MemoryBase = 0;
    }
    // Set up our aux values here
    AuxVariables.emplace_back(auxv_t{3, MemoryBase}); // Program header
    AuxVariables.emplace_back(auxv_t{7, MemoryBase}); // Interpreter address
    AuxVariables.emplace_back(auxv_t{9, MemoryBase + DB.DefaultRIP()}); // AT_ENTRY
    AuxVariables.emplace_back(auxv_t{0, 0}); // Null ender
  }

  template <typename PointerType, typename AuxType, size_t PointerSize>
  static void SetupPointers(
    uintptr_t StackPointer,
    uint64_t rsp,
    uint64_t AuxVOffset,
    uint64_t ArgumentOffset,
    uint64_t EnvpOffset,
    const std::vector<std::string> &Args,
    const std::vector<std::string> &EnvironmentVariables,
    const std::vector<auxv_t> &AuxVariables,
    uint64_t AuxTabBase,
    uint64_t AuxTabSize,
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
    PointerType ArgumentBackingBaseGuest = rsp + ArgumentOffset;
    PointerType EnvpBackingBaseGuest = rsp + EnvpOffset;

    *reinterpret_cast<PointerType *>(StackPointer + 0) = Args.size();
    PadPointers[0] = 0;

    // If we don't have any, just make sure the first is nullptr
    EnvpPointers[0] = 0;

    uint64_t CurrentOffset = 0;
    for (size_t i = 0; i < Args.size(); ++i) {
      size_t ArgSize = Args[i].size();
      // Set the pointer to this argument
      ArgumentPointers[i] = ArgumentBackingBaseGuest + CurrentOffset;
      // Copy the string in to the final location
      memcpy(reinterpret_cast<void*>(ArgumentBackingBase + CurrentOffset), &Args[i].at(0), ArgSize);

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
        AuxType Random{25, static_cast<PointerType>(rsp + RandomNumberOffset)};
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

    *(PointerType*)(&AuxTabBase) = static_cast<PointerType>(reinterpret_cast<uintptr_t>(AuxVPointers));
    *(PointerType*)(&AuxTabSize) = sizeof(AuxType) * AuxVariables.size();
  }

  uint64_t SetupStack(void *HostPtr, uint64_t GuestPtr) const override {
    uintptr_t StackPointer = reinterpret_cast<uintptr_t>(HostPtr) + StackSize();
    // Set up our initial CPU state
    uint64_t rsp = GuestPtr + StackSize();
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
    rsp -= TotalArgumentMemSize;
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
        rsp,
        AuxVOffset,
        ArgumentOffset,
        EnvpOffset,
        Args,
        EnvironmentVariables,
        AuxVariables,
        AuxTabBase,
        AuxTabSize,
        RandomNumberLocation
        );
    }
    else {
      SetupPointers<uint32_t, auxv32_t, 4>(
        StackPointer,
        rsp,
        AuxVOffset,
        ArgumentOffset,
        EnvpOffset,
        Args,
        EnvironmentVariables,
        AuxVariables,
        AuxTabBase,
        AuxTabSize,
        RandomNumberLocation
        );
    }

    return rsp;
  }

  uint64_t DefaultRIP() const override {
    return MemoryBase + DB.DefaultRIP();
  }

  void MapMemoryRegion(std::function<void*(uint64_t, uint64_t, bool, bool)> Mapper) override {
    DB.MapMemoryRegions(Mapper);
  }

  void LoadMemory(MemoryWriter Writer) override {
    auto ELFLoaderWrapper = [&](void const *Data, uint64_t Addr, uint64_t Size) -> void {
      Writer(Data, MemoryBase + Addr, Size);
    };
    DB.WriteLoadableSections(ELFLoaderWrapper);
  }

  char const *FindSymbolNameInRange(uint64_t Address) override {
    ELFLoader::ELFSymbol const *Sym;
    Sym = DB.GetSymbolInRange(std::make_pair(Address, 1));
    if (Sym) {
      return Sym->Name;
    }
    return nullptr;
  }

  void GetInitLocations(std::vector<uint64_t> *Locations) override {
    DB.GetInitLocations(Locations);
  }

  uint64_t InitializeThreadSlot(std::function<void(void const*, uint64_t)> Writer) const override {
    return DB.InitializeThreadSlot(Writer);
  };

  void GetExecveArguments(std::vector<char const*> *Args) override { *Args = LoaderArgs; }

  void GetAuxv(uint64_t& addr, uint64_t& size) override {
    addr = AuxTabBase;
    size = AuxTabSize;
  }

  bool Is64BitMode() const { return File.GetMode() == ::ELFLoader::ELFContainer::MODE_64BIT; }

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
  uint64_t MemoryBase{};

  constexpr static uint64_t STACK_SIZE = 8 * 1024 * 1024;
};

}
