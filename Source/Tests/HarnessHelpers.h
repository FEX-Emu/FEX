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
#include <sys/user.h>
#include <vector>

#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/BitUtils.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXHeaderUtils/Syscalls.h>
#include <FEXHeaderUtils/TypeDefines.h>

#include <fmt/format.h>

namespace FEX::HarnessHelper {
  inline bool CompareStates(FEXCore::Core::CPUState const& State1,
       FEXCore::Core::CPUState const& State2,
       uint64_t MatchMask,
       bool OutputGPRs) {
    bool Matches = true;

    const auto DumpGPRs = [OutputGPRs](const std::string& Name, uint64_t A, uint64_t B) {
      if (!OutputGPRs) {
        return;
      }
      if (A == B) {
        return;
      }

      fmt::print("{}: 0x{:016x} {} 0x{:016x}\n", Name, A, A==B ? "==" : "!=", B);
    };

    const auto DumpFLAGs = [OutputGPRs](const std::string& Name, uint64_t A, uint64_t B) {
      if (!OutputGPRs) {
        return;
      }
      if (A == B) {
        return;
      }

      static constexpr std::array<uint32_t, 17> Flags = {
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

      fmt::print("{}: 0x{:016x} {} 0x{:016x}\n", Name, A, A==B ? "==" : "!=", B);
      for (const auto Flag : Flags) {
        const auto FlagMask = uint64_t{1} << Flag;
        if ((A & FlagMask) != (B & FlagMask)) {
          fmt::print("\t{}: {} != {}\n", FEXCore::Core::GetFlagName(Flag), (A >> Flag) & 1, (B >> Flag) & 1);
        }
      }
    };

    const auto CheckGPRs = [&Matches, DumpGPRs](const std::string& Name, uint64_t A, uint64_t B){
      DumpGPRs(Name, A, B);
      Matches &= A == B;
    };

    const auto CheckFLAGS = [&Matches, DumpFLAGs](const std::string& Name, uint64_t A, uint64_t B){
      DumpFLAGs(Name, A, B);
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
    std::fstream TestFile(Filename, std::fstream::in | std::fstream::binary);
    LOGMAN_THROW_A_FMT(TestFile.is_open(), "Failed to open file");

    TestFile.seekg(0, std::fstream::end);
    const size_t FileSize = TestFile.tellg();
    TestFile.seekg(0, std::fstream::beg);

    Data->resize(FileSize);

    TestFile.read(Data->data(), FileSize);
  }

  class ConfigLoader final {
  public:
    void Init(std::string const &ConfigFilename) {
      ReadFile(ConfigFilename, &RawConfigFile);
      memcpy(&BaseConfig, RawConfigFile.data(), sizeof(ConfigStructBase));
      GetEnvironmentOptions();
    }

    std::vector<std::pair<std::string_view, std::string_view>> GetEnvironmentOptions() {
      std::vector<std::pair<std::string_view, std::string_view>> Env{};

      uintptr_t DataOffset = BaseConfig.OptionEnvOptionOffset;
      for (unsigned i = 0; i < BaseConfig.OptionEnvOptionCount; ++i) {
        // Environment variables are null terminated strings
        std::string_view Key = RawConfigFile.data() + DataOffset;
        std::string_view Value = RawConfigFile.data() + DataOffset + Key.size() + 1;
        DataOffset += Key.size() + Value.size() + 2;
        Env.emplace_back(Key, Value);
      }
      return Env;
    }

    bool CompareStates(FEXCore::Core::CPUState const* State1, FEXCore::Core::CPUState const* State2) {
      bool Matches = true;
      uint64_t MatchMask = BaseConfig.OptionMatch & ~BaseConfig.OptionIgnore;
      if (State1 && State2) {
        Matches &= FEX::HarnessHelper::CompareStates(*State1, *State2, MatchMask, ConfigDumpGPRs());
      }

      if (BaseConfig.OptionRegDataCount > 0) {
        static constexpr std::array<uint64_t, 45> OffsetArray = {{
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

          size_t NameIndex = FEXCore::FindFirstSetBit(RegData->RegKey) - 1;
          auto Offset = OffsetArray[NameIndex];
          uint64_t *State1Data = reinterpret_cast<uint64_t*>(reinterpret_cast<uint64_t>(State1) + Offset);
          uint64_t *State2Data = reinterpret_cast<uint64_t*>(reinterpret_cast<uint64_t>(State2) + Offset);

          const auto DumpGPRs = [this](const std::string& Name, uint64_t A, uint64_t B) {
            if (!ConfigDumpGPRs()) {
              return;
            }

            fmt::print("{}: 0x{:016x} {} 0x{:016x} (Expected)\n", Name, A, A==B ? "==" : "!=", B);
          };

          const auto CheckGPRs = [&Matches, DumpGPRs](const std::string& Name, uint64_t A, uint64_t B) {
            DumpGPRs(Name, A, B);
            Matches &= A == B;
          };

          for (size_t j = 0; j < RegData->RegDataCount; ++j) {
            std::string Name;
            if (NameIndex == 0) // RIP
              Name = "RIP";
            else if (NameIndex >= 1 && NameIndex < 17)
              Name = fmt::format("GPR{}", NameIndex - 1);
            else if (NameIndex >= 17 && NameIndex < 33)
              Name = fmt::format("XMM[{}][{}]", NameIndex - 17, j);
            else if (NameIndex == 33)
              Name = "gs";
            else if (NameIndex == 34)
              Name ="fs";
            else if (NameIndex == 35)
              Name = "rflags";
            else if (NameIndex >= 36 && NameIndex < 45)
              Name = fmt::format("MM[{}][{}]", NameIndex - 36, j);

            if (State1) {
              CheckGPRs(fmt::format("Core1: {}: ", Name), State1Data[j], RegData->RegValues[j]);
            }
            if (State2) {
              CheckGPRs(fmt::format("Core2: {}: ", Name), State2Data[j], RegData->RegValues[j]);
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

    void LoadMemory() {
      uintptr_t DataOffset = BaseConfig.OptionMemDataOffset;
      for (unsigned i = 0; i < BaseConfig.OptionMemDataCount; ++i) {
        MemDataStructBase *MemData = reinterpret_cast<MemDataStructBase*>(RawConfigFile.data() + DataOffset);
        memcpy(reinterpret_cast<void*>(MemData->address), &MemData->data, MemData->length);
        DataOffset += sizeof(MemDataStructBase) + MemData->length;
      }
    }

    bool Is64BitMode() const { return BaseConfig.OptionMode == 1; }

  private:
    FEX_CONFIG_OPT(ConfigDumpGPRs, DUMPGPRS);

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
      uint32_t OptionEnvOptionOffset;
      uint32_t OptionEnvOptionCount;
      uint8_t  AdditionalData[];
    } FEX_PACKED;

    struct MemoryRegionBase {
      uint64_t Region;
      uint64_t Size;
    } FEX_PACKED;

    struct RegDataStructBase {
      uint32_t RegDataCount;
      uint64_t RegKey;
      uint64_t RegValues[];
    } FEX_PACKED;

    struct MemDataStructBase {
      uint64_t address;
      uint32_t length;
      uint8_t data[];
    } FEX_PACKED;

    std::vector<char> RawConfigFile;
    ConfigStructBase BaseConfig;
  };

  class HarnessCodeLoader final : public FEXCore::CodeLoader {
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

    uint64_t GetStackPointer() override {
      if (Config.Is64BitMode()) {
        return reinterpret_cast<uint64_t>(FEXCore::Allocator::mmap(nullptr, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) + STACK_SIZE;
      }
      else {
        uint64_t Result = reinterpret_cast<uint64_t>(FEXCore::Allocator::mmap(reinterpret_cast<void*>(STACK_OFFSET), STACK_SIZE, PROT_READ | PROT_WRITE, MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        LOGMAN_THROW_A_FMT(Result != ~0ULL, "Stack Pointer mmap failed");
        return Result + STACK_SIZE;
      }
    }

    uint64_t DefaultRIP() const override {
      return RIP;
    }

    bool MapMemory(const MapperFn& Mapper, const UnmapperFn& Unmapper) override {
      bool LimitedSize = true;
      auto DoMMap = [&Mapper](uint64_t Address, size_t Size) -> void* {
        void *Result = Mapper(reinterpret_cast<void*>(Address), Size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        LOGMAN_THROW_A_FMT(Result == reinterpret_cast<void*>(Address), "Map Memory mmap failed");
        return Result;
      };

      if (LimitedSize) {
        DoMMap(0xe000'0000, FHU::FEX_PAGE_SIZE * 10);

        // SIB8
        // We test [-128, -126] (Bottom)
        // We test [-8, 8] (Middle)
        // We test [120, 127] (Top)
        // Can fit in two pages
        DoMMap(0xe800'0000 - FHU::FEX_PAGE_SIZE, FHU::FEX_PAGE_SIZE * 2);
      }
      else {
        // This is scratch memory location and SIB8 location
        DoMMap(0xe000'0000, 0x1000'0000);
        // This is for large SIB 32bit displacement testing
        DoMMap(0x2'0000'0000, 0x1'0000'1000);
      }

      // Map in the memory region for the test file
      size_t Length = FEXCore::AlignUp(RawFile.size(), FHU::FEX_PAGE_SIZE);
      Code_start_page = reinterpret_cast<uint64_t>(DoMMap(Code_start_page, Length));
      mprotect(reinterpret_cast<void*>(Code_start_page), Length, PROT_READ | PROT_WRITE | PROT_EXEC);
      RIP = Code_start_page;

      // Map the memory regions the test file asks for
      for (auto& [region, size] : Config.GetMemoryRegions()) {
        DoMMap(region, size);
      }

      LoadMemory();

      return true;
    }

    void LoadMemory() {
      // Memory base here starts at the start location we passed back with GetLayout()
      // This will write at [CODE_START_RANGE + 0, RawFile.size() )
      memcpy(reinterpret_cast<void*>(RIP), &RawFile.at(0), RawFile.size());
      Config.LoadMemory();
    }

    std::vector<std::pair<std::string_view, std::string_view>> GetEnvironmentOptions() {
      return Config.GetEnvironmentOptions();
    }

    bool CompareStates(FEXCore::Core::CPUState const* State1, FEXCore::Core::CPUState const* State2) {
      return Config.CompareStates(State1, State2);
    }

    bool Is64BitMode() const { return Config.Is64BitMode(); }

  private:
    constexpr static uint64_t STACK_SIZE = FHU::FEX_PAGE_SIZE;
    constexpr static uint64_t STACK_OFFSET = 0xc000'0000;
    // Zero is special case to know when we are done
    uint64_t Code_start_page = 0x1'0000;
    uint64_t RIP {};

    std::vector<char> RawFile;
    ConfigLoader Config;
  };

}
