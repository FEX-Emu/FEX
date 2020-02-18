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
#include <vector>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>

namespace FEX::HarnessHelper {
   bool CompareStates(FEXCore::Core::CPUState const& State1,
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

  void ReadFile(std::string const &Filename, std::vector<char> *Data) {
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

    bool CompareStates(FEXCore::Core::CPUState const& State1, FEXCore::Core::CPUState const& State2) {
      bool Matches = true;
      uint64_t MatchMask = BaseConfig.OptionMatch & ~BaseConfig.OptionIgnore;
      Matches &= FEX::HarnessHelper::CompareStates(State1, State2, MatchMask, ConfigDumpGPRs());

      if (BaseConfig.OptionRegDataCount > 0) {
        uintptr_t DataOffset = sizeof(ConfigStructBase);
        constexpr std::array<std::pair<uint64_t, unsigned>, 45> OffsetArray = {{
          {offsetof(FEXCore::Core::CPUState, rip), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[0]), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[1]), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[2]), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[3]), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[4]), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[5]), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[6]), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[7]), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[8]), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[9]), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[10]), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[11]), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[12]), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[13]), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[14]), 1},
          {offsetof(FEXCore::Core::CPUState, gregs[15]), 1},
          {offsetof(FEXCore::Core::CPUState, xmm[0][0]), 2},
          {offsetof(FEXCore::Core::CPUState, xmm[1][0]), 2},
          {offsetof(FEXCore::Core::CPUState, xmm[2][0]), 2},
          {offsetof(FEXCore::Core::CPUState, xmm[3][0]), 2},
          {offsetof(FEXCore::Core::CPUState, xmm[4][0]), 2},
          {offsetof(FEXCore::Core::CPUState, xmm[5][0]), 2},
          {offsetof(FEXCore::Core::CPUState, xmm[6][0]), 2},
          {offsetof(FEXCore::Core::CPUState, xmm[7][0]), 2},
          {offsetof(FEXCore::Core::CPUState, xmm[8][0]), 2},
          {offsetof(FEXCore::Core::CPUState, xmm[9][0]), 2},
          {offsetof(FEXCore::Core::CPUState, xmm[10][0]), 2},
          {offsetof(FEXCore::Core::CPUState, xmm[11][0]), 2},
          {offsetof(FEXCore::Core::CPUState, xmm[12][0]), 2},
          {offsetof(FEXCore::Core::CPUState, xmm[13][0]), 2},
          {offsetof(FEXCore::Core::CPUState, xmm[14][0]), 2},
          {offsetof(FEXCore::Core::CPUState, xmm[15][0]), 2},
          {offsetof(FEXCore::Core::CPUState, gs), 1},
          {offsetof(FEXCore::Core::CPUState, fs), 1},
          {offsetof(FEXCore::Core::CPUState, flags), 8},
          {offsetof(FEXCore::Core::CPUState, mm[0][0]), 2},
          {offsetof(FEXCore::Core::CPUState, mm[1][0]), 2},
          {offsetof(FEXCore::Core::CPUState, mm[2][0]), 2},
          {offsetof(FEXCore::Core::CPUState, mm[3][0]), 2},
          {offsetof(FEXCore::Core::CPUState, mm[4][0]), 2},
          {offsetof(FEXCore::Core::CPUState, mm[5][0]), 2},
          {offsetof(FEXCore::Core::CPUState, mm[6][0]), 2},
          {offsetof(FEXCore::Core::CPUState, mm[7][0]), 2},
          {offsetof(FEXCore::Core::CPUState, mm[8][0]), 2},
        }};

        // Offset past the Memory regions if there are any
        DataOffset += sizeof(MemoryRegionBase) * BaseConfig.OptionMemoryRegionCount;
        for (unsigned i = 0; i < BaseConfig.OptionRegDataCount; ++i) {
          RegDataStructBase *RegData = reinterpret_cast<RegDataStructBase*>(RawConfigFile.data() + DataOffset);
          [[maybe_unused]] std::bitset<64> RegFlags = RegData->RegKey;
          assert(RegFlags.count() == 1  && "Must set reg data explicitly per register");
          assert(RegData->RegKey != (1UL << 36) && "Invalid register selection");

          size_t NameIndex = __builtin_ffsl(RegData->RegKey)- 1;
          auto Offset = OffsetArray[NameIndex];
          uint64_t *State1Data = reinterpret_cast<uint64_t*>(reinterpret_cast<uint64_t>(&State1) + Offset.first);
          uint64_t *State2Data = reinterpret_cast<uint64_t*>(reinterpret_cast<uint64_t>(&State2) + Offset.first);

          auto DumpGPRs = [this](auto Name, uint64_t A, uint64_t B) {
            if (!ConfigDumpGPRs())
              return;

            printf("%s: 0x%016lx %s 0x%016lx\n", Name.c_str(), A, A==B ? "==" : "!=", B);
          };

          auto CheckGPRs = [&Matches, DumpGPRs](std::string Name, uint64_t A, uint64_t B) {
            DumpGPRs(std::move(Name), A, B);
            Matches &= A == B;
          };

          for (unsigned j = 0; j < Offset.second; ++j) {
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


            CheckGPRs("Core1: " + Name + ": ", State1Data[j], RegData->RegValues[j]);
            CheckGPRs("Core2: " + Name + ": ", State2Data[j], RegData->RegValues[j]);
          }

          // Get the correct data offset
          DataOffset += sizeof(RegDataStructBase) + Offset.second * 8;
        }
      }
      return Matches;
    }

  private:
    FEX::Config::Value<bool> ConfigDumpGPRs{"DumpGPRs", false};

    struct ConfigStructBase {
      uint64_t OptionMatch;
      uint64_t OptionIgnore;
      uint64_t OptionStackSize;
      uint64_t OptionEntryPoint;
      uint32_t OptionABI;
      uint32_t OptionMemoryRegionCount;
      uint32_t OptionRegDataCount;
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

    uint64_t SetupStack([[maybe_unused]] void *HostPtr, uint64_t GuestPtr) const override {
      return GuestPtr + STACK_SIZE - 16;
    }

    uint64_t DefaultRIP() const override {
      return RIP;
    }

    MemoryLayout GetLayout() const override {
      uint64_t CodeSize = RawFile.size();
      CodeSize = AlignUp(CodeSize, PAGE_SIZE);
      return std::make_tuple(CODE_START_RANGE, CODE_START_RANGE + CodeSize, CodeSize);
    }

    void MapMemoryRegion(std::function<void*(uint64_t, uint64_t)> Mapper) override {
      bool LimitedSize = true;
      if (LimitedSize) {
        Mapper(0xe000'0000, PAGE_SIZE * 10);

        // SIB8
        // We test [-128, -126] (Bottom)
        // We test [-8, 8] (Middle)
        // We test [120, 127] (Top)
        // Can fit in two pages
        Mapper(0xe800'0000 - PAGE_SIZE, PAGE_SIZE * 2);

        // SIB32 Bottom
        // We test INT_MIN, INT_MIN + 8
        Mapper(0x2'0000'0000, PAGE_SIZE);
        // SIB32 Middle
        // We test -8 + 8
        Mapper(0x2'8000'0000 - PAGE_SIZE, PAGE_SIZE * 2);

        // SIB32 Top
        // We Test INT_MAX - 8, INT_MAX
        Mapper(0x3'0000'0000 - PAGE_SIZE, PAGE_SIZE * 2);
      }
      else {
        // This is scratch memory location and SIB8 location
        Mapper(0xe000'0000, 0x1000'0000);
        // This is for large SIB 32bit displacement testing
        Mapper(0x2'0000'0000, 0x1'0000'1000);
      }

      // Map in the memory region for the test file
      Mapper(CODE_START_PAGE, AlignUp(RawFile.size(), PAGE_SIZE));
    }

    void LoadMemory(MemoryWriter Writer) override {
      // Memory base here starts at the start location we passed back with GetLayout()
      // This will write at [CODE_START_RANGE + 0, RawFile.size() )
      Writer(&RawFile.at(0), CODE_START_RANGE, RawFile.size());
    }

    uint64_t GetFinalRIP() override { return CODE_START_RANGE + RawFile.size(); }

    bool CompareStates(FEXCore::Core::CPUState const& State1, FEXCore::Core::CPUState const& State2) {
      return Config.CompareStates(State1, State2);
    }

  private:
    constexpr static uint64_t STACK_SIZE = PAGE_SIZE;
    // Zero is special case to know when we are done
    constexpr static uint64_t CODE_START_PAGE = 0x0'1000;
    constexpr static uint64_t CODE_START_RANGE = CODE_START_PAGE + 0x1;
    constexpr static uint64_t RIP = CODE_START_RANGE;

    std::vector<char> RawFile;
    ConfigLoader Config;
  };

class ELFCodeLoader final : public FEXCore::CodeLoader {
public:
  ELFCodeLoader(std::string const &Filename, [[maybe_unused]] std::vector<std::string> const &args, char **const envp = nullptr)
    : File {Filename, false}
    , DB {&File}
    , Args {args} {

    if (File.WasDynamic()) {
      // If the file isn't static then we need to add the filename of interpreter
      // to the front of the argument list
      Args.emplace(Args.begin(), File.InterpreterLocation());
    }

    // Just block pretty much everything
    const std::vector<const char*> NotAllowed = {
      "XDG_CONFIG_DIRS",
      "XDG_SEAT",
      "XDG_SESSION_DESKTOP",
      "XDG_SESSION_TYPE",
      "XDG_CURRENT_DESKTOP",
      "XDG_SESSION_CLASS",
      "XDG_VTNR",
      "XDG_SESSION_ID",
      "XDG_RUNTIME_DIR",
      "XDG_DATA_DIRS",

      "QT_ACCESSIBILITY",
      "COLORTERM",
      "DERBY_HOME",
      "TERMCAP",
      "JAVA_HOME",
      "SSH_AUTH_SOCK",
      "DESKTOP_SESSION",
      "SSH_AGENT_PID",
      "GTK_MODULES",
      "LOGNAME",
      "GPG_AGENT_INFO",
      "XAUTHORITY",
      "J2REDIR",
      "WINDOWPATH",
      "HOME",
      "LANG",
      "LS_COLORS",
      "PM_PACKAGES_ROOT",
      "TERM",
      "DEFAULT_PATH",
      "J2SDKDIR",
      "LESSCLOSE",
      "LESSOPEN",
      "LIBVIRT_DEFAULT_URI",
      "USER",
      "DISPLAY",
      "SHLVL",
      "PATH",
      "STY",
      "GDMSESSION",
      "DBUS_SESSION_BUS_ADDRESS",
      "OLDPWD",
      "_",

      "WINIT_HIDPI_FACTOR",
      "SHELL",
      "WINDOWID",
      "MANDATORY_PATH",
      "WINDOW",
      "PWD",
      "DEFAULTS_PATH",

      "ALACRITTY_LOG",
      "USERNAME",
    };

    if (!!envp) {
      // If we had envp passed in then make sure to set it up on the guest
      for (unsigned i = 0;; ++i) {
        if (envp[i] == nullptr)
          break;
        bool Invalid = false;

        for (auto Str : NotAllowed) {
          if (strncmp(Str, envp[i], strlen(Str)) == 0) {
            Invalid = true;
            break;
          }
        }

        if (Invalid) continue;

        EnvironmentVariables.emplace_back(envp[i]);
      }
    }

    // Calculate argument and envp backing sizes
     for (unsigned i = 0; i < Args.size(); ++i) {
      ArgumentBackingSize += Args[i].size() + 1;
    }

    for (unsigned i = 0; i < EnvironmentVariables.size(); ++i) {
      EnvironmentBackingSize += EnvironmentVariables[i].size() + 1;
    }
  }

  uint64_t StackSize() const override {
    return STACK_SIZE;
  }

  uint64_t SetupStack(void *HostPtr, uint64_t GuestPtr) const override {
    uintptr_t StackPointer = reinterpret_cast<uintptr_t>(HostPtr) + StackSize();
    // Set up our initial CPU state
    uint64_t rsp = GuestPtr + StackSize();

    uint64_t TotalArgumentMemSize{};

    TotalArgumentMemSize += 8; // Argument counter size
    TotalArgumentMemSize += 8 * Args.size(); // Pointers to strings
    TotalArgumentMemSize += 8; // Padding for something
    TotalArgumentMemSize += 8 * EnvironmentVariables.size(); // Argument location for envp
    TotalArgumentMemSize += 8; // Padding for something
    TotalArgumentMemSize += 8; // Padding for something

    uint64_t ArgumentOffset = TotalArgumentMemSize;
    TotalArgumentMemSize += ArgumentBackingSize;

    uint64_t EnvpOffset = TotalArgumentMemSize;
    TotalArgumentMemSize += EnvironmentBackingSize;

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

    // Pointer list offsets
    uint64_t *ArgumentPointers = reinterpret_cast<uint64_t*>(StackPointer + 8);
    uint64_t *PadPointers = reinterpret_cast<uint64_t*>(StackPointer + 8 + Args.size() * 8);
    uint64_t *EnvpPointers = reinterpret_cast<uint64_t*>(StackPointer + 8 + Args.size() * 8 + 8);

    // Arguments memory lives after everything else
    uint8_t *ArgumentBackingBase = reinterpret_cast<uint8_t*>(StackPointer + ArgumentOffset);
    uint8_t *EnvpBackingBase = reinterpret_cast<uint8_t*>(StackPointer + EnvpOffset);
    uint64_t ArgumentBackingBaseGuest = rsp + ArgumentOffset;
    uint64_t EnvpBackingBaseGuest = rsp + EnvpOffset;

    *reinterpret_cast<uint64_t*>(StackPointer + 0) = Args.size();
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

    return rsp;
  }

  uint64_t DefaultRIP() const override {
    return DB.DefaultRIP();
  }

  void MapMemoryRegion(std::function<void*(uint64_t, uint64_t)> Mapper) override {
    DB.MapMemoryRegions(Mapper);
  }

  MemoryLayout GetLayout() const override {
    return DB.GetFileLayout();
  }

  void LoadMemory(MemoryWriter Writer) override {
    DB.WriteLoadableSections(Writer);
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

private:
  ::ELFLoader::ELFContainer File;
  ::ELFLoader::ELFSymbolDatabase DB;
  std::vector<std::string> Args;
  std::vector<std::string> EnvironmentVariables;
  uint64_t ArgumentBackingSize{};
  uint64_t EnvironmentBackingSize{};

  constexpr static uint64_t STACK_SIZE = 8 * 1024 * 1024;
};

}
