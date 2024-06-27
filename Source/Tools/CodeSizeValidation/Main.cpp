// SPDX-License-Identifier: MIT
#include "DummyHandlers.h"
#include "FEXCore/Core/Context.h"
#include "FEXCore/Debug/InternalThreadState.h"
#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/File.h>
#include <FEXCore/Utils/FileLoading.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/SignalScopeGuards.h>

#include <sys/stat.h>

namespace CodeSize {
class CodeSizeValidation final {
public:
  struct InstructionStats {
    uint64_t GuestCodeInstructions {};
    uint64_t HostCodeInstructions {};

    uint64_t HeaderSize {};
    uint64_t TailSize {};
  };

  using CodeLines = fextl::vector<fextl::string>;
  using InstructionData = std::pair<InstructionStats, CodeLines>;

  bool ParseMessage(const char* Message);

  InstructionData* GetDataForRIP(uint64_t RIP) {
    return &RIPToStats[RIP];
  }

  bool InfoPrintingDisabled() const {
    return SetupInfoDisabled;
  }

  void CalculateBaseStats(FEXCore::Context::Context* CTX, FEXCore::Core::InternalThreadState* Thread);
private:
  void ClearStats() {
    RIPToStats.clear();
  }

  void SetBaseStats(const InstructionStats& NewBase) {
    BaseStats = NewBase;
  }

  void CalculateDifferenceBetweenStats(InstructionData* Nop, InstructionData* Fence);

  uint64_t CurrentRIPParse {};
  bool ConsumingDisassembly {};
  InstructionData* CurrentStats {};
  InstructionStats BaseStats {};

  fextl::unordered_map<uint64_t, InstructionData> RIPToStats;
  bool SetupInfoDisabled {};
};

constexpr std::string_view RIPMessage = "RIP: 0x";
constexpr std::string_view GuestCodeMessage = "Guest Code instructions: ";
constexpr std::string_view HostCodeMessage = "Host Code instructions: ";
constexpr std::string_view DisassembleBeginMessage = "Disassemble Begin";
constexpr std::string_view DisassembleEndMessage = "Disassemble End";
constexpr std::string_view BlowUpMsg = "Blow-up Amt: ";

static std::string_view SanitizeDisassembly(std::string_view Message) {
  auto it = Message.find(" (addr");
  // If it contains an address calculation, strip it out.
  Message = Message.substr(0, it);
  if (Message.find("adrp ") != std::string_view::npos || Message.find("adr ") != std::string_view::npos) {
    Message = Message.substr(0, Message.find(" #"));
  }
  return Message;
}

bool CodeSizeValidation::ParseMessage(const char* Message) {
  // std::string_view doesn't have contains until c++23.
  std::string_view MessageView {Message};
  if (MessageView.find(RIPMessage) != MessageView.npos) {
    // New RIP found
    std::string_view RIPView = std::string_view {Message + RIPMessage.size()};
    std::from_chars(RIPView.data(), RIPView.end(), CurrentRIPParse, 16);
    CurrentStats = &RIPToStats[CurrentRIPParse];
    return false;
  }

  if (MessageView.find(GuestCodeMessage) != MessageView.npos) {
    std::string_view CodeSizeView = std::string_view {Message + GuestCodeMessage.size()};
    std::from_chars(CodeSizeView.data(), CodeSizeView.end(), CurrentStats->first.GuestCodeInstructions);
    return false;
  }
  if (MessageView.find(HostCodeMessage) != MessageView.npos) {
    std::string_view CodeSizeView = std::string_view {Message + HostCodeMessage.size()};
    std::from_chars(CodeSizeView.data(), CodeSizeView.end(), CurrentStats->first.HostCodeInstructions);

    CurrentStats->first.HostCodeInstructions -= BaseStats.HostCodeInstructions;
    return false;
  }
  if (MessageView.find(DisassembleBeginMessage) != MessageView.npos) {
    ConsumingDisassembly = true;
    // Just so the output isn't a mess.
    return false;
  }
  if (MessageView.find(DisassembleEndMessage) != MessageView.npos) {
    ConsumingDisassembly = false;
    // Just so the output isn't a mess.

    // Remove the header and tails.
    if (BaseStats.HeaderSize) {
      CurrentStats->second.erase(CurrentStats->second.begin(), CurrentStats->second.begin() + BaseStats.HeaderSize);
    }
    if (BaseStats.TailSize) {
      CurrentStats->second.erase(CurrentStats->second.end() - BaseStats.TailSize, CurrentStats->second.end());
    }
    return false;
  }

  if (MessageView.find(BlowUpMsg) != MessageView.npos) {
    return false;
  }

  if (ConsumingDisassembly) {
    // Currently consuming disassembly. Each line will be a single line of disassembly.
    CurrentStats->second.push_back(fextl::string(SanitizeDisassembly(Message)));
    return false;
  }

  return true;
}

void CodeSizeValidation::CalculateDifferenceBetweenStats(InstructionData* Nop, InstructionData* Fence) {
  // Expected format.
  // adr x0, #-0x4 (addr 0x7fffe9880054)
  // str x0, [x28, #184]
  // dmb sy
  // ldr x0, pc+8 (addr 0x7fffe988006c)
  // blr x0
  // unallocated (Unallocated)
  // udf #0x7fff
  // unallocated (Unallocated)
  // udf #0x0
  //
  // First two lines are the header.
  // Next comes the implementation (0 instruction size for nop, 1 instruction for fence)
  // After that is the tail.

  const auto& NOPCode = Nop->second;
  const auto& FENCECode = Fence->second;

  LOGMAN_THROW_A_FMT(NOPCode.size() < FENCECode.size(), "NOP code must be smaller than fence!");
  for (size_t i = 0; i < NOPCode.size(); ++i) {
    const auto& NOPLine = NOPCode.at(i);
    const auto& FENCELine = FENCECode.at(i);

    const auto NOPmnemonic = std::string_view(NOPLine.data(), NOPLine.find(' '));
    const auto FENCEmnemonic = std::string_view(FENCELine.data(), FENCELine.find(' '));

    if (NOPmnemonic != FENCEmnemonic) {
      // Headersize of a block is now `i` number of instructions.
      Nop->first.HeaderSize = i;

      // Tail size is going to be the remaining size
      Nop->first.TailSize = NOPCode.size() - i;
      break;
    }
  }

  SetBaseStats(Nop->first);
}

void CodeSizeValidation::CalculateBaseStats(FEXCore::Context::Context* CTX, FEXCore::Core::InternalThreadState* Thread) {
  SetupInfoDisabled = true;

  // Known hardcoded instructions that will generate blocks of particular sizes.
  // NOP will never generate any instructions.
  constexpr static uint8_t NOP[] = {
    0x90,
  };

  // MFENCE will always generate a block with one instruction.
  constexpr static uint8_t MFENCE[] = {
    0x0f,
    0xae,
    0xf0,
  };

  // Compile the NOP.
  CTX->CompileRIP(Thread, (uint64_t)NOP);
  // Gather the stats for the NOP.
  auto NOPStats = GetDataForRIP((uint64_t)NOP);

  // Compile MFence
  CTX->CompileRIP(Thread, (uint64_t)MFENCE);

  // Get MFence stats.
  auto MFENCEStats = GetDataForRIP((uint64_t)MFENCE);

  // Now scan the difference in disasembly between NOP and MFENCE to remove the header and tail.
  // Just searching for first instruction change.

  CalculateDifferenceBetweenStats(NOPStats, MFENCEStats);
  // Now that the stats have been cleared. Clear our currentStats.
  ClearStats();

  // Invalidate the code ranges to be safe.
  auto CodeInvalidationlk = FEXCore::GuardSignalDeferringSection(CTX->GetCodeInvalidationMutex(), Thread);
  CTX->InvalidateGuestCodeRange(Thread, (uint64_t)NOP, sizeof(NOP));
  CTX->InvalidateGuestCodeRange(Thread, (uint64_t)MFENCE, sizeof(MFENCE));
  SetupInfoDisabled = false;
}

static CodeSizeValidation Validation {};
} // namespace CodeSize

void MsgHandler(LogMan::DebugLevels Level, const char* Message) {
  const char* CharLevel {LogMan::DebugLevelStr(Level)};

  if (Level == LogMan::INFO) {
    // Disassemble information is sent through the Info log level.
    if (!CodeSize::Validation.ParseMessage(Message)) {
      return;
    }
    if (CodeSize::Validation.InfoPrintingDisabled()) {
      return;
    }
  }

  fextl::fmt::print("[{}] {}\n", CharLevel, Message);
}

void AssertHandler(const char* Message) {
  fextl::fmt::print("[ASSERT] {}\n", Message);

  // make sure buffers are flushed
  fflush(nullptr);
}

struct TestInfo {
  char TestInst[128];
  int64_t ExpectedInstructionCount;
  uint64_t CodeSize;
  uint64_t x86InstCount;
  uint32_t Cookie;
  uint8_t Code[];
};

struct TestHeader {
  uint64_t Bitness;
  uint64_t NumTests {};
  uint64_t EnabledHostFeatures;
  uint64_t DisabledHostFeatures;
  uint64_t EnvironmentVariableCount;
  uint8_t Data[];
};

static void* TestData;
static size_t TestDataSize;
static const TestHeader* TestHeaderData {};
static const TestInfo* TestsStart {};
static fextl::vector<std::pair<std::string_view, std::string_view>> EnvironmentVariables {};

static bool TestInstructions(FEXCore::Context::Context* CTX, FEXCore::Core::InternalThreadState* Thread, const char* UpdatedInstructionCountsPath) {
  LogMan::Msg::IFmt("Compiling code");

  // Tell FEXCore to compile all the instructions upfront.
  const TestInfo* CurrentTest = TestsStart;
  for (size_t i = 0; i < TestHeaderData->NumTests; ++i) {
    uint64_t CodeRIP = (uint64_t)&CurrentTest->Code[0];
    LogMan::Msg::IFmt("Compiling instruction '{}'", CurrentTest->TestInst);

    // Compile the INST.
    CTX->CompileRIPCount(Thread, CodeRIP, CurrentTest->x86InstCount);

    // Go to the next test.
    CurrentTest = reinterpret_cast<const TestInfo*>(&CurrentTest->Code[CurrentTest->CodeSize]);
  }

  bool TestsPassed {true};

  // Get all the data for the instructions compiled.
  CurrentTest = TestsStart;
  for (size_t i = 0; i < TestHeaderData->NumTests; ++i) {
    uint64_t CodeRIP = (uint64_t)CurrentTest->Code;
    // Get the instruction stats.
    auto INSTStats = CodeSize::Validation.GetDataForRIP(CodeRIP);

    LogMan::Msg::IFmt("Testing instruction '{}': {} host instructions", CurrentTest->TestInst, INSTStats->first.HostCodeInstructions);

    // Show the code if the count of instructions changed to something we didn't expect.
    bool ShouldShowCode = INSTStats->first.HostCodeInstructions != CurrentTest->ExpectedInstructionCount;

    if (ShouldShowCode) {
      for (auto Line : INSTStats->second) {
        LogMan::Msg::EFmt("\t{}", Line);
      }
    }

    if (INSTStats->first.HostCodeInstructions != CurrentTest->ExpectedInstructionCount) {
      LogMan::Msg::EFmt("Fail: '{}': {} host instructions", CurrentTest->TestInst, INSTStats->first.HostCodeInstructions);
      LogMan::Msg::EFmt("Fail: Test took {} instructions but we expected {} instructions!", INSTStats->first.HostCodeInstructions,
                        CurrentTest->ExpectedInstructionCount);

      // Fail the test if the instruction count has changed at all.
      TestsPassed = false;
    }

    // Go to the next test.
    CurrentTest = reinterpret_cast<const TestInfo*>(&CurrentTest->Code[CurrentTest->CodeSize]);
  }

  if (UpdatedInstructionCountsPath) {
    // Unlink the file.
    unlink(UpdatedInstructionCountsPath);

    FEXCore::File::File FD(UpdatedInstructionCountsPath,
                           FEXCore::File::FileModes::WRITE | FEXCore::File::FileModes::CREATE | FEXCore::File::FileModes::TRUNCATE);

    if (!FD.IsValid()) {
      // If we couldn't open the file then early exit this.
      LogMan::Msg::EFmt("Couldn't open {} for updating instruction counts", UpdatedInstructionCountsPath);
      return TestsPassed;
    }

    FD.Write("{\n", 2);

    CurrentTest = TestsStart;
    for (size_t i = 0; i < TestHeaderData->NumTests; ++i) {
      uint64_t CodeRIP = (uint64_t)CurrentTest->Code;
      // Get the instruction stats.
      auto INSTStats = CodeSize::Validation.GetDataForRIP(CodeRIP);

      FD.Write(fextl::fmt::format("\t\"{}\": {{\n", CurrentTest->TestInst));

      if (INSTStats->first.HostCodeInstructions != CurrentTest->ExpectedInstructionCount) {
        FD.Write(fextl::fmt::format("\t\t\"ExpectedInstructionCount\": {},\n", INSTStats->first.HostCodeInstructions));
      }

      FD.Write(fextl::fmt::format("\t\t\"ExpectedArm64ASM\": [\n", INSTStats->first.HostCodeInstructions));
      for (auto it = INSTStats->second.begin(); it != INSTStats->second.end(); ++it) {
        const auto& Line = *it;
        const auto NextIt = it + 1;
        FD.Write(fextl::fmt::format("\t\t\t\"{}\"{}\n", Line, NextIt != INSTStats->second.end() ? "," : ""));
      }
      FD.Write(fextl::fmt::format("\t\t]\n", INSTStats->first.HostCodeInstructions));

      FD.Write(fextl::fmt::format("\t}},\n", CurrentTest->TestInst));

      // Go to the next test.
      CurrentTest = reinterpret_cast<const TestInfo*>(&CurrentTest->Code[CurrentTest->CodeSize]);
    }

    // Print a null member
    FD.Write(fextl::fmt::format("\t\"\": \"\""));

    FD.Write("}\n", 2);
  }
  return TestsPassed;
}

bool LoadTests(const char* Path) {
  constexpr uint64_t Code_start_page = 0x1'0000;

  int FD = open(Path, O_RDONLY | O_CLOEXEC);
  if (FD == -1) {
    return false;
  }

  struct stat buf;
  if (fstat(FD, &buf) == -1) {
    close(FD);
    return false;
  }

  TestDataSize = buf.st_size;
  TestData = FEXCore::Allocator::mmap(reinterpret_cast<void*>(Code_start_page), TestDataSize, PROT_READ, MAP_PRIVATE, FD, 0);
  if (reinterpret_cast<uint64_t>(TestData) == ~0ULL) {
    close(FD);
    return false;
  }

  if (reinterpret_cast<uint64_t>(TestData) != Code_start_page) {
    FEXCore::Allocator::VirtualFree(TestData, TestDataSize);
    close(FD);
    return false;
  }

  close(FD);

  TestHeaderData = reinterpret_cast<const TestHeader*>(TestData);

  // Need to walk past the environment variables to get to the actual tests.
  const uint8_t* Data = TestHeaderData->Data;
  for (size_t i = 0; i < TestHeaderData->EnvironmentVariableCount; ++i) {
    // Environment variables are a pair of null terminated strings.
    Data += strlen(reinterpret_cast<const char*>(Data)) + 1;
    Data += strlen(reinterpret_cast<const char*>(Data)) + 1;
  }
  TestsStart = reinterpret_cast<const TestInfo*>(Data);
  return true;
}

namespace {
static const fextl::vector<std::pair<const char*, FEXCore::Config::ConfigOption>> EnvConfigLookup = {{
#define OPT_BASE(type, group, enum, json, default) {"FEX_" #enum, FEXCore::Config::ConfigOption::CONFIG_##enum},
#include <FEXCore/Config/ConfigValues.inl>
}};

// Claims to be a local application config layer
class TestEnvLoader final : public FEXCore::Config::Layer {
public:
  explicit TestEnvLoader()
    : FEXCore::Config::Layer(FEXCore::Config::LayerType::LAYER_LOCAL_APP) {
    Load();
  }

  void Load() override {
    fextl::unordered_map<std::string_view, std::string> EnvMap;
    const uint8_t* Data = TestHeaderData->Data;
    for (size_t i = 0; i < TestHeaderData->EnvironmentVariableCount; ++i) {
      // Environment variables are a pair of null terminated strings.
      const std::string_view Key = reinterpret_cast<const char*>(Data);
      Data += strlen(reinterpret_cast<const char*>(Data)) + 1;

      const std::string_view Value_View = reinterpret_cast<const char*>(Data);
      Data += strlen(reinterpret_cast<const char*>(Data)) + 1;
      std::optional<fextl::string> Value;

#define ENVLOADER
#include <FEXCore/Config/ConfigOptions.inl>

      if (Value) {
        EnvMap.insert_or_assign(Key, *Value);
      } else {
        EnvMap.insert_or_assign(Key, Value_View);
      }
    }

    auto GetVar = [&](const std::string_view id) -> std::optional<std::string_view> {
      const auto it = EnvMap.find(id);
      if (it == EnvMap.end()) {
        return std::nullopt;
      }

      return it->second;
    };

    for (auto& it : EnvConfigLookup) {
      if (auto Value = GetVar(it.first); Value) {
        Set(it.second, *Value);
      }
    }
  }

private:
  fextl::vector<std::pair<std::string_view, std::string_view>> Env;
};
} // namespace

int main(int argc, char** argv, char** const envp) {
  FEXCore::Allocator::GLIBCScopedFault GLIBFaultScope;
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);
  FEXCore::Config::Initialize();
  FEXCore::Config::Load();

  if (argc < 2) {
    LogMan::Msg::EFmt("Usage: {} <Test binary> [Changed instruction count.json]", argv[0]);
    return 1;
  }

  if (!LoadTests(argv[1])) {
    LogMan::Msg::EFmt("Couldn't load tests from {}", argv[1]);
    return 1;
  }

  FEXCore::Config::AddLayer(fextl::make_unique<TestEnvLoader>());
  FEXCore::Config::ReloadMetaLayer();

  // Setup configurations that this tool needs
  // Maximum one instruction.
  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_MAXINST, "1");
  // IRJIT. Only works on JITs.
  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_CORE, fextl::fmt::format("{}", static_cast<uint64_t>(FEXCore::Config::CONFIG_IRJIT)));
  // Enable block disassembly.
  FEXCore::Config::EraseSet(
    FEXCore::Config::CONFIG_DISASSEMBLE,
    fextl::fmt::format("{}", static_cast<uint64_t>(FEXCore::Config::Disassemble::BLOCKS | FEXCore::Config::Disassemble::STATS)));
  // Choose bitness.
  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_IS64BIT_MODE, TestHeaderData->Bitness == 64 ? "1" : "0");
  // Disable telemetry, it can affect instruction counts.
  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_DISABLETELEMETRY, "1");
  // Disable vixl simulator indirect calls as it can affect instruction counts.
  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_DISABLE_VIXL_INDIRECT_RUNTIME_CALLS, "1");

  // Host feature override. Only supports overriding SVE width.
  enum HostFeatures {
    FEATURE_SVE128 = (1U << 0),
    FEATURE_SVE256 = (1U << 1),
    FEATURE_CLZERO = (1U << 2),
    FEATURE_RNG = (1U << 3),
    FEATURE_FCMA = (1U << 4),
    FEATURE_CSSC = (1U << 5),
    FEATURE_AFP = (1U << 6),
    FEATURE_RPRES = (1U << 7),
    FEATURE_FLAGM = (1U << 8),
    FEATURE_FLAGM2 = (1U << 9),
    FEATURE_CRYPTO = (1U << 10),
  };

  uint64_t SVEWidth = 0;
  uint64_t HostFeatureControl {};
  if (TestHeaderData->EnabledHostFeatures & FEATURE_SVE128) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::ENABLESVE);
    SVEWidth = 128;
  }
  if (TestHeaderData->EnabledHostFeatures & FEATURE_SVE256) {
    SVEWidth = 256;
  }
  if (TestHeaderData->EnabledHostFeatures & FEATURE_CLZERO) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::ENABLECLZERO);
  }
  if (TestHeaderData->EnabledHostFeatures & FEATURE_RNG) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::ENABLERNG);
  }
  if (TestHeaderData->EnabledHostFeatures & FEATURE_FCMA) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::ENABLEFCMA);
  }
  if (TestHeaderData->EnabledHostFeatures & FEATURE_CSSC) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::ENABLECSSC);
  }
  if (TestHeaderData->EnabledHostFeatures & FEATURE_AFP) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::ENABLEAFP);
  }
  if (TestHeaderData->EnabledHostFeatures & FEATURE_RPRES) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::ENABLERPRES);
  }
  if (TestHeaderData->EnabledHostFeatures & FEATURE_FLAGM) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::ENABLEFLAGM);
  }
  if (TestHeaderData->EnabledHostFeatures & FEATURE_FLAGM2) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::ENABLEFLAGM2);
  }
  if (TestHeaderData->EnabledHostFeatures & FEATURE_CRYPTO) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::ENABLECRYPTO);
  }

  // Always enable ARMv8.1 LSE atomics.
  HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::ENABLEATOMICS);

  if (TestHeaderData->DisabledHostFeatures & FEATURE_SVE128) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::DISABLESVE);
  }
  if (TestHeaderData->DisabledHostFeatures & FEATURE_CLZERO) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::DISABLECLZERO);
  }
  if (TestHeaderData->DisabledHostFeatures & FEATURE_RNG) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::DISABLERNG);
  }
  if (TestHeaderData->DisabledHostFeatures & FEATURE_FCMA) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::DISABLEFCMA);
  }
  if (TestHeaderData->DisabledHostFeatures & FEATURE_CSSC) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::DISABLECSSC);
  }
  if (TestHeaderData->DisabledHostFeatures & FEATURE_AFP) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::DISABLEAFP);
  }
  if (TestHeaderData->DisabledHostFeatures & FEATURE_RPRES) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::DISABLERPRES);
  }
  if (TestHeaderData->DisabledHostFeatures & FEATURE_FLAGM) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::DISABLEFLAGM);
  }
  if (TestHeaderData->DisabledHostFeatures & FEATURE_FLAGM2) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::DISABLEFLAGM2);
  }
  if (TestHeaderData->DisabledHostFeatures & FEATURE_CRYPTO) {
    HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::DISABLECRYPTO);
  }

  // Always enable preserve_all abi.
  HostFeatureControl |= static_cast<uint64_t>(FEXCore::Config::HostFeatures::ENABLEPRESERVEALLABI);

  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_HOSTFEATURES, fextl::fmt::format("{}", HostFeatureControl));
  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_FORCESVEWIDTH, fextl::fmt::format("{}", SVEWidth));

  // Initialize static tables.
  FEXCore::Context::InitializeStaticTables(TestHeaderData->Bitness == 64 ? FEXCore::Context::MODE_64BIT : FEXCore::Context::MODE_32BIT);

  // Create FEXCore context.
  auto CTX = FEXCore::Context::Context::CreateNewContext();

  auto SignalDelegation = FEX::DummyHandlers::CreateSignalDelegator();
  auto SyscallHandler = FEX::DummyHandlers::CreateSyscallHandler();

  CTX->SetSignalDelegator(SignalDelegation.get());
  CTX->SetSyscallHandler(SyscallHandler.get());
  if (!CTX->InitCore()) {
    return -1;
  }
  auto ParentThread = CTX->CreateThread(0, 0);

  // Calculate the base stats for instruction testing.
  CodeSize::Validation.CalculateBaseStats(CTX.get(), ParentThread);

  // Test all the instructions.
  auto Result = TestInstructions(CTX.get(), ParentThread, argc >= 2 ? argv[2] : nullptr) ? 0 : 1;
  CTX->DestroyThread(ParentThread);

  FEXCore::Allocator::VirtualFree(TestData, TestDataSize);
  return Result;
}
