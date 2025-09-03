// SPDX-License-Identifier: MIT
#pragma once

#include "CodeLoader.h"
#include "Common/Config.h"

#include <array>
#include <bitset>
#include <cassert>
#include <cstring>
#include <fcntl.h>

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/FileLoading.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/TypeDefines.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/BitUtils.h>
#include <FEXHeaderUtils/Syscalls.h>
#include <unistd.h>

namespace FEX::HarnessHelper {
inline bool CompareStates(const FEXCore::Core::CPUState& State1, const FEXCore::Core::CPUState& State2, uint64_t MatchMask, bool OutputGPRs,
                          bool SupportsAVX) {
  bool Matches = true;

  const auto DumpGPRs = [OutputGPRs](const fextl::string& Name, uint64_t A, uint64_t B) {
    if (!OutputGPRs) {
      return;
    }
    if (A == B) {
      return;
    }

    fextl::fmt::print("{}: 0x{:016x} {} 0x{:016x}\n", Name, A, A == B ? "==" : "!=", B);
  };

  const auto CheckGPRs = [&Matches, DumpGPRs](const fextl::string& Name, uint64_t A, uint64_t B) {
    DumpGPRs(Name, A, B);
    Matches &= A == B;
  };

  // RIP
  if (MatchMask & 1) {
    CheckGPRs("RIP", State1.rip, State2.rip);
  }

  MatchMask >>= 1;

  // GPRS
  for (unsigned i = 0; i < FEXCore::Core::CPUState::NUM_GPRS; ++i, MatchMask >>= 1) {
    if (MatchMask & 1) {
      CheckGPRs(fextl::fmt::format("GPR{}", i), State1.gregs[i], State2.gregs[i]);
    }
  }

  // XMM
  if (SupportsAVX) {
    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_XMMS; ++i, MatchMask >>= 1) {
      if (MatchMask & 1) {
        CheckGPRs(fextl::fmt::format("XMM0_{}", i), State1.xmm.avx.data[i][0], State2.xmm.avx.data[i][0]);
        CheckGPRs(fextl::fmt::format("XMM1_{}", i), State1.xmm.avx.data[i][1], State2.xmm.avx.data[i][1]);
      }
    }
  } else {
    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_XMMS; ++i, MatchMask >>= 1) {
      if (MatchMask & 1) {
        CheckGPRs(fextl::fmt::format("XMM0_{}", i), State1.xmm.sse.data[i][0], State2.xmm.sse.data[i][0]);
        CheckGPRs(fextl::fmt::format("XMM1_{}", i), State1.xmm.sse.data[i][1], State2.xmm.sse.data[i][1]);
      }
    }
  }

  // GS
  if (MatchMask & 1) {
    CheckGPRs("GS", State1.gs_cached, State2.gs_cached);
  }
  MatchMask >>= 1;

  // FS
  if (MatchMask & 1) {
    CheckGPRs("FS", State1.fs_cached, State2.fs_cached);
  }

  return Matches;
}

class ConfigLoader final {
public:
  void Init(const fextl::string& ConfigFilename) {
    FEXCore::FileLoading::LoadFile(RawConfigFile, ConfigFilename);
    memcpy(&BaseConfig, RawConfigFile.data(), sizeof(ConfigStructBase));
    GetEnvironmentOptions();
  }

  fextl::vector<std::pair<std::string_view, std::string_view>> GetEnvironmentOptions() {
    fextl::vector<std::pair<std::string_view, std::string_view>> Env {};

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

  bool CompareStates(const FEXCore::Core::CPUState* State1, const FEXCore::Core::CPUState* State2, bool SupportsAVX) {
    bool Matches = true;
    uint64_t MatchMask = BaseConfig.OptionMatch & ~BaseConfig.OptionIgnore;
    if (State1 && State2) {
      Matches &= FEX::HarnessHelper::CompareStates(*State1, *State2, MatchMask, ConfigDumpGPRs(), SupportsAVX);
    }

    if (BaseConfig.OptionRegDataCount > 0) {
      static constexpr std::array<uint64_t, 43> OffsetArrayAVX = {{
        offsetof(FEXCore::Core::CPUState, rip),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBX]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBP]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R8]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R9]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R10]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R11]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R12]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R13]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R14]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R15]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[0][0]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[1][0]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[2][0]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[3][0]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[4][0]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[5][0]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[6][0]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[7][0]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[8][0]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[9][0]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[10][0]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[11][0]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[12][0]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[13][0]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[14][0]),
        offsetof(FEXCore::Core::CPUState, xmm.avx.data[15][0]),
        offsetof(FEXCore::Core::CPUState, gs_cached),
        offsetof(FEXCore::Core::CPUState, fs_cached),
        offsetof(FEXCore::Core::CPUState, mm[0][0]),
        offsetof(FEXCore::Core::CPUState, mm[1][0]),
        offsetof(FEXCore::Core::CPUState, mm[2][0]),
        offsetof(FEXCore::Core::CPUState, mm[3][0]),
        offsetof(FEXCore::Core::CPUState, mm[4][0]),
        offsetof(FEXCore::Core::CPUState, mm[5][0]),
        offsetof(FEXCore::Core::CPUState, mm[6][0]),
        offsetof(FEXCore::Core::CPUState, mm[7][0]),
      }};
      static constexpr std::array<uint64_t, 43> OffsetArraySSE = {{
        offsetof(FEXCore::Core::CPUState, rip),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RAX]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBX]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RCX]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDX]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSI]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RDI]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RBP]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_RSP]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R8]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R9]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R10]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R11]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R12]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R13]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R14]),
        offsetof(FEXCore::Core::CPUState, gregs[FEXCore::X86State::REG_R15]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[0][0]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[1][0]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[2][0]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[3][0]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[4][0]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[5][0]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[6][0]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[7][0]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[8][0]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[9][0]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[10][0]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[11][0]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[12][0]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[13][0]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[14][0]),
        offsetof(FEXCore::Core::CPUState, xmm.sse.data[15][0]),
        offsetof(FEXCore::Core::CPUState, gs_cached),
        offsetof(FEXCore::Core::CPUState, fs_cached),
        offsetof(FEXCore::Core::CPUState, mm[0][0]),
        offsetof(FEXCore::Core::CPUState, mm[1][0]),
        offsetof(FEXCore::Core::CPUState, mm[2][0]),
        offsetof(FEXCore::Core::CPUState, mm[3][0]),
        offsetof(FEXCore::Core::CPUState, mm[4][0]),
        offsetof(FEXCore::Core::CPUState, mm[5][0]),
        offsetof(FEXCore::Core::CPUState, mm[6][0]),
        offsetof(FEXCore::Core::CPUState, mm[7][0]),
      }};

      uintptr_t DataOffset = BaseConfig.OptionRegDataOffset;
      for (unsigned i = 0; i < BaseConfig.OptionRegDataCount; ++i) {
        RegDataStructBase* RegData = reinterpret_cast<RegDataStructBase*>(RawConfigFile.data() + DataOffset);
        [[maybe_unused]] std::bitset<64> RegFlags = RegData->RegKey;
        assert(RegFlags.count() == 1 && "Must set reg data explicitly per register");

        size_t NameIndex = FEXCore::FindFirstSetBit(RegData->RegKey) - 1;
        auto Offset = SupportsAVX ? OffsetArrayAVX[NameIndex] : OffsetArraySSE[NameIndex];
        uint64_t* State1Data = reinterpret_cast<uint64_t*>(reinterpret_cast<uint64_t>(State1) + Offset);
        uint64_t* State2Data = reinterpret_cast<uint64_t*>(reinterpret_cast<uint64_t>(State2) + Offset);

        const auto DumpGPRs = [this](const fextl::string& Name, uint64_t A, uint64_t B) {
          if (!ConfigDumpGPRs()) {
            return;
          }

          fextl::fmt::print("{}: 0x{:016x} {} 0x{:016x} (Expected)\n", Name, A, A == B ? "==" : "!=", B);
        };

        const auto CheckGPRs = [&Matches, DumpGPRs](const fextl::string& Name, uint64_t A, uint64_t B) {
          DumpGPRs(Name, A, B);
          Matches &= A == B;
        };

        for (size_t j = 0; j < RegData->RegDataCount; ++j) {
          fextl::string Name;
          if (NameIndex == 0) { // RIP
            Name = "RIP";
          } else if (NameIndex >= 1 && NameIndex < 17) {
            Name = fextl::fmt::format("GPR{}", NameIndex - 1);
          } else if (NameIndex >= 17 && NameIndex < 33) {
            Name = fextl::fmt::format("XMM[{}][{}]", NameIndex - 17, j);
          } else if (NameIndex == 33) {
            Name = "gs";
          } else if (NameIndex == 34) {
            Name = "fs";
          } else if (NameIndex >= 35 && NameIndex < 43) {
            Name = fextl::fmt::format("MM[{}][{}]", NameIndex - 35, j);
          }

          if (State1) {
            CheckGPRs(fextl::fmt::format("Core1: {}: ", Name), State1Data[j], RegData->RegValues[j]);
          }
          if (State2) {
            CheckGPRs(fextl::fmt::format("Core2: {}: ", Name), State2Data[j], RegData->RegValues[j]);
          }
        }

        // Get the correct data offset
        DataOffset += sizeof(RegDataStructBase) + RegData->RegDataCount * 8;
      }
    }
    return Matches;
  }

  fextl::map<uintptr_t, size_t> GetMemoryRegions() {
    fextl::map<uintptr_t, size_t> regions;

    uintptr_t DataOffset = BaseConfig.OptionMemoryRegionOffset;
    for (unsigned i = 0; i < BaseConfig.OptionMemoryRegionCount; ++i) {
      MemoryRegionBase* Region = reinterpret_cast<MemoryRegionBase*>(RawConfigFile.data() + DataOffset);
      regions[Region->Region] = Region->Size;

      DataOffset += sizeof(MemoryRegionBase);
    }

    return regions;
  }

  void LoadMemory() {
    uintptr_t DataOffset = BaseConfig.OptionMemDataOffset;
    for (unsigned i = 0; i < BaseConfig.OptionMemDataCount; ++i) {
      MemDataStructBase* MemData = reinterpret_cast<MemDataStructBase*>(RawConfigFile.data() + DataOffset);
      memcpy(reinterpret_cast<void*>(MemData->address), &MemData->data, MemData->length);
      DataOffset += sizeof(MemDataStructBase) + MemData->length;
    }
  }

  bool Is64BitMode() const {
    return BaseConfig.OptionMode == 1;
  }

  enum HostFeatures {
    FEATURE_ANY = 0,
    FEATURE_3DNOW = (1 << 0),
    FEATURE_SSE4A = (1 << 1),
    FEATURE_AVX = (1 << 2),
    FEATURE_RAND = (1 << 3),
    FEATURE_SHA = (1 << 4),
    FEATURE_CLZERO = (1 << 5),
    FEATURE_BMI1 = (1 << 6),
    FEATURE_BMI2 = (1 << 7),
    FEATURE_CLWB = (1 << 8),
    FEATURE_LINUX = (1 << 9),
    FEATURE_AES256 = (1 << 10),
    FEATURE_AFP = (1 << 11),
    FEATURE_SSSE3 = (1 << 12),
    FEATURE_SSE4_1 = (1 << 13),
    FEATURE_SSE4_2 = (1 << 14),
    FEATURE_AES = (1 << 15),
    FEATURE_PCLMUL = (1 << 16),
    FEATURE_MOVBE = (1 << 17),
    FEATURE_ADX = (1 << 18),
    FEATURE_XSAVE = (1 << 19),
    FEATURE_RDPID = (1 << 20),
    FEATURE_CLFLOPT = (1 << 21),
    FEATURE_FSGSBASE = (1 << 22),
    FEATURE_EMMI = (1 << 23),
  };

  bool Requires3DNow() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_3DNOW;
  }
  bool RequiresSSE4A() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_SSE4A;
  }
  bool RequiresAVX() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_AVX;
  }
  bool RequiresRAND() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_RAND;
  }
  bool RequiresSHA() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_SHA;
  }
  bool RequiresCLZERO() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_CLZERO;
  }
  bool RequiresBMI1() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_BMI1;
  }
  bool RequiresBMI2() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_BMI2;
  }
  bool RequiresCLWB() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_CLWB;
  }
  bool RequiresLinux() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_LINUX;
  }
  bool RequiresAES256() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_AES256;
  }
  bool RequiresAFP() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_AFP;
  }
  bool RequiresSSSE3() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_SSSE3;
  }
  bool RequiresSSE4_1() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_SSE4_1;
  }
  bool RequiresSSE4_2() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_SSE4_2;
  }
  bool RequiresAES() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_AES;
  }
  bool RequiresPCLMUL() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_PCLMUL;
  }
  bool RequiresMOVBE() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_MOVBE;
  }
  bool RequiresADX() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_ADX;
  }
  bool RequiresXSAVE() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_XSAVE;
  }
  bool RequiresRDPID() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_RDPID;
  }
  bool RequiresCLFLOPT() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_CLFLOPT;
  }
  bool RequiresFSGSBase() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_FSGSBASE;
  }
  bool RequiresEMMI() const {
    return BaseConfig.OptionHostFeatures & HostFeatures::FEATURE_EMMI;
  }

private:
  FEX_CONFIG_OPT(ConfigDumpGPRs, DUMPGPRS);

  struct ConfigStructBase {
    uint64_t OptionMatch;
    uint64_t OptionIgnore;
    uint64_t OptionStackSize;
    uint64_t OptionEntryPoint;
    uint32_t OptionABI;
    uint32_t OptionMode;
    uint32_t OptionHostFeatures;
    uint32_t OptionMemoryRegionOffset;
    uint32_t OptionMemoryRegionCount;
    uint32_t OptionRegDataOffset;
    uint32_t OptionRegDataCount;
    uint32_t OptionMemDataOffset;
    uint32_t OptionMemDataCount;
    uint32_t OptionEnvOptionOffset;
    uint32_t OptionEnvOptionCount;
    uint8_t AdditionalData[];
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

  fextl::vector<char> RawConfigFile;
  ConfigStructBase BaseConfig;
};

class HarnessCodeLoader final : public FEX::CodeLoader {
public:

  HarnessCodeLoader(const fextl::string& Filename, const fextl::string& ConfigFilename) {
    FEXCore::FileLoading::LoadFile(RawASMFile, Filename);

    Config.Init(ConfigFilename);
  }

  uint64_t StackSize() const override {
    const auto Page = sysconf(_SC_PAGESIZE);
    return Page > 0 ? Page : FEXCore::Utils::FEX_PAGE_SIZE;
  }

  uint64_t GetStackPointer() override {
    LOGMAN_MSG_A_FMT("This should be unused.");
    FEX_UNREACHABLE;
  }

  uint64_t DefaultRIP() const override {
    return RIP;
  }

  bool MapMemory(const std::function<void*(uint64_t, size_t)>& DoMMap) {
    bool LimitedSize = true;
    auto AllocPageSize = sysconf(_SC_PAGESIZE);
    if (AllocPageSize <= 0) {
      AllocPageSize = FEXCore::Utils::FEX_PAGE_SIZE;
    }

    if (LimitedSize) {
      DoMMap(0xe000'0000, AllocPageSize * 10);

      // SIB8
      // We test [-128, -126] (Bottom)
      // We test [-8, 8] (Middle)
      // We test [120, 127] (Top)
      // Can fit in two pages
      DoMMap(0xe800'0000 - AllocPageSize, AllocPageSize * 2);
    } else {
      // This is scratch memory location and SIB8 location
      DoMMap(0xe000'0000, 0x1000'0000);
      // This is for large SIB 32bit displacement testing
      DoMMap(0x2'0000'0000, 0x1'0000'1000);
    }

    // Map in the memory region for the test file
#ifndef _WIN32
    size_t Length = FEXCore::AlignUp(RawASMFile.size(), FEXCore::Utils::FEX_PAGE_SIZE);
    auto ASMPtr = DoMMap(Code_start_page, Length);
#else
    // Special magic DOS area that starts at 0x1'0000
    auto ASMPtr = DoMMap(1, 0x110000 - 1);
#endif
    LOGMAN_THROW_A_FMT((uint64_t)ASMPtr == Code_start_page, "Couldn't allocate code at expected page: 0x{:x} != 0x{:x}", (uint64_t)ASMPtr,
                       Code_start_page);
    memcpy(ASMPtr, RawASMFile.data(), RawASMFile.size());
    RIP = Code_start_page;

    // Map the memory regions the test file asks for
    for (auto& [region, size] : Config.GetMemoryRegions()) {
      DoMMap(region, size);
    }

    if (!Config.Is64BitMode()) {
      // 32-bit gets a fixed page allocated for stack.
      DoMMap(STACK_OFFSET, StackSize());
    }

    LoadMemory();

    return true;
  }

  void LoadMemory() {
    // Memory base here starts at the start location we passed back with GetLayout()
    // This will write at [CODE_START_RANGE + 0, RawFile.size() )
    Config.LoadMemory();
  }

  fextl::vector<std::pair<std::string_view, std::string_view>> GetEnvironmentOptions() {
    return Config.GetEnvironmentOptions();
  }

  bool CompareStates(const FEXCore::Core::CPUState* State1, const FEXCore::Core::CPUState* State2, bool SupportsAVX) {
    return Config.CompareStates(State1, State2, SupportsAVX);
  }

  bool Is64BitMode() const {
    return Config.Is64BitMode();
  }
  bool Requires3DNow() const {
    return Config.Requires3DNow();
  }
  bool RequiresSSE4A() const {
    return Config.RequiresSSE4A();
  }
  bool RequiresAVX() const {
    return Config.RequiresAVX();
  }
  bool RequiresRAND() const {
    return Config.RequiresRAND();
  }
  bool RequiresSHA() const {
    return Config.RequiresSHA();
  }
  bool RequiresCLZERO() const {
    return Config.RequiresCLZERO();
  }
  bool RequiresBMI1() const {
    return Config.RequiresBMI1();
  }
  bool RequiresBMI2() const {
    return Config.RequiresBMI2();
  }
  bool RequiresCLWB() const {
    return Config.RequiresCLWB();
  }
  bool RequiresLinux() const {
    return Config.RequiresLinux();
  }
  bool RequiresAES256() const {
    return Config.RequiresAES256();
  }
  bool RequiresAFP() const {
    return Config.RequiresAFP();
  }
  bool RequiresSSSE3() const {
    return Config.RequiresSSSE3();
  }
  bool RequiresSSE4_1() const {
    return Config.RequiresSSE4_1();
  }
  bool RequiresSSE4_2() const {
    return Config.RequiresSSE4_2();
  }
  bool RequiresAES() const {
    return Config.RequiresAES();
  }
  bool RequiresPCLMUL() const {
    return Config.RequiresPCLMUL();
  }
  bool RequiresMOVBE() const {
    return Config.RequiresMOVBE();
  }
  bool RequiresADX() const {
    return Config.RequiresADX();
  }
  bool RequiresXSAVE() const {
    return Config.RequiresXSAVE();
  }
  bool RequiresRDPID() const {
    return Config.RequiresRDPID();
  }
  bool RequiresCLFLOPT() const {
    return Config.RequiresCLFLOPT();
  }
  bool RequiresFSGSBase() const {
    return Config.RequiresFSGSBase();
  }
  bool RequiresEMMI() const {
    return Config.RequiresEMMI();
  }

private:
  constexpr static uint64_t STACK_OFFSET = 0xc000'0000;
  // Zero is special case to know when we are done
  uint64_t Code_start_page = 0x1'0000;
  uint64_t RIP {};

  fextl::vector<char> RawASMFile;
  ConfigLoader Config;
};

} // namespace FEX::HarnessHelper
