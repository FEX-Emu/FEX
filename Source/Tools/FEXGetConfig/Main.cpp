// SPDX-License-Identifier: MIT
#include "Common/cpp-optparse/OptionParser.h"
#include "Common/Config.h"
#include "Common/FEXServerClient.h"
#include "FEXHeaderUtils/Filesystem.h"
#include "git_version.h"
#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>
#include <FEXHeaderUtils/Filesystem.h>

#include <cstdio>
#include <filesystem>
#include <string>
#include <sys/prctl.h>

namespace {
struct TSOEmulationFacts {
  bool LSE {}, LSE2 {};
  bool HardwareTSO {};
  bool LRCPC1 {}, LRCPC2 {}, LRCPC3 {};
};

#ifdef _M_ARM_64
bool CheckForHardwareTSO() {
  // We need to check if these are defined or not. This is a very fresh feature.
#ifndef PR_GET_MEM_MODEL
#define PR_GET_MEM_MODEL 0x6d4d444c
#endif
#ifndef PR_SET_MEM_MODEL
#define PR_SET_MEM_MODEL 0x4d4d444c
#endif
#ifndef PR_SET_MEM_MODEL_DEFAULT
#define PR_SET_MEM_MODEL_DEFAULT 0
#endif
#ifndef PR_SET_MEM_MODEL_TSO
#define PR_SET_MEM_MODEL_TSO 1
#endif
  // Check to see if this is supported.
  auto Result = prctl(PR_GET_MEM_MODEL, 0, 0, 0, 0);
  if (Result == -1) {
    // Unsupported, early exit.
    return false;
  }

  if (Result == PR_SET_MEM_MODEL_DEFAULT) {
    // Try to set the TSO mode if we are currently default.
    Result = prctl(PR_SET_MEM_MODEL, PR_SET_MEM_MODEL_TSO, 0, 0, 0);
    if (Result == 0) {
      Result = prctl(PR_SET_MEM_MODEL, PR_SET_MEM_MODEL_DEFAULT, 0, 0, 0);
      return true;
    }
  }
  return false;
}

enum ISAR0_FIELDS {
  LSE = 20,
};

enum ISAR1_FIELDS {
  LRCPC = 20,
};

enum MMFR2_FIELDS {
  AT = 32,
};

constexpr static uint32_t IDFIELDMASK = 0b1111;
uint64_t GetISAR0() {
  uint64_t Result {};
  asm("mrs %0, ID_AA64ISAR0_EL1;" : "=r"(Result));
  return Result;
}

uint64_t GetISAR1() {
  uint64_t Result {};
  asm("mrs %0, ID_AA64ISAR1_EL1;" : "=r"(Result));
  return Result;
}

uint64_t GetMMFR2() {
  uint64_t Result {};
  asm("mrs %0, ID_AA64MMFR2_EL1;" : "=r"(Result));
  return Result;
}

TSOEmulationFacts GetTSOEmulationFacts() {
  const auto ISAR0 = GetISAR0();
  const auto ISAR1 = GetISAR1();
  const auto MMFR2 = GetMMFR2();

  return {
    .LSE = ((ISAR0 >> ISAR0_FIELDS::LSE) & IDFIELDMASK) >= 0b0010,
    .LSE2 = ((MMFR2 >> MMFR2_FIELDS::AT) & IDFIELDMASK) >= 0b0001,
    .HardwareTSO = CheckForHardwareTSO(),
    .LRCPC1 = ((ISAR1 >> ISAR1_FIELDS::LRCPC) & IDFIELDMASK) >= 0b0001,
    .LRCPC2 = ((ISAR1 >> ISAR1_FIELDS::LRCPC) & IDFIELDMASK) >= 0b0010,
    .LRCPC3 = ((ISAR1 >> ISAR1_FIELDS::LRCPC) & IDFIELDMASK) >= 0b0011,
  };
}
#else
TSOEmulationFacts GetTSOEmulationFacts() {
  return {};
}
#endif
} // namespace

int main(int argc, char** argv, char** envp) {
  FEX::Config::InitializeConfigs(FEX::Config::PortableInformation {});
  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(FEX::Config::CreateGlobalMainLayer());
  FEXCore::Config::AddLayer(FEX::Config::CreateMainLayer());
  // No FEX arguments passed through command line
  FEXCore::Config::AddLayer(FEX::Config::CreateEnvironmentLayer(envp));

  // Load the arguments
  optparse::OptionParser Parser = optparse::OptionParser().description("Simple application to get a couple of FEX options");

  Parser.add_option("--install-prefix").action("store_true").help("Print the FEX install prefix");

  Parser.add_option("--app").help("Load an application profile for this application if it exists");

  Parser.add_option("--current-rootfs").action("store_true").help("Print the directory that contains the FEX rootfs. Mounted in the case of squashfs");

  Parser.add_option("--tso-emulation-info").action("store_true").help("Print how FEX is emulating the x86-TSO memory model.");

  Parser.add_option("--version").action("store_true").help("Print the installed FEX-Emu version");

  optparse::Values Options = Parser.parse_args(argc, argv);

  if (Options.is_set_by_user("app")) {
    // Load the application config if one was provided
    const auto ProgramName = FHU::Filesystem::GetFilename(Options["app"]);
    FEXCore::Config::AddLayer(FEX::Config::CreateAppLayer(ProgramName, FEXCore::Config::LayerType::LAYER_GLOBAL_APP));
    FEXCore::Config::AddLayer(FEX::Config::CreateAppLayer(ProgramName, FEXCore::Config::LayerType::LAYER_LOCAL_APP));

    auto SteamID = getenv("SteamAppId");
    if (SteamID) {
      // If a SteamID exists then let's search for Steam application configs as well.
      // We want to key off both the SteamAppId number /and/ the executable since we may not want to thunk all binaries.
      const auto SteamAppName = fextl::fmt::format("Steam_{}_{}", SteamID, ProgramName);
      FEXCore::Config::AddLayer(FEX::Config::CreateAppLayer(SteamAppName, FEXCore::Config::LayerType::LAYER_GLOBAL_STEAM_APP));
      FEXCore::Config::AddLayer(FEX::Config::CreateAppLayer(SteamAppName, FEXCore::Config::LayerType::LAYER_LOCAL_STEAM_APP));
    }
  }

  FEXCore::Config::Load();

  // Reload the meta layer
  FEXCore::Config::ReloadMetaLayer();

  if (Options.is_set_by_user("version")) {
    fprintf(stdout, GIT_DESCRIBE_STRING "\n");
  }

  if (Options.is_set_by_user("install_prefix")) {
    char SelfPath[PATH_MAX];
    auto Result = readlink("/proc/self/exe", SelfPath, PATH_MAX);
    if (Result == -1) {
      Result = 0;
    }
    auto InstallPrefix = std::filesystem::path(&SelfPath[0], &SelfPath[Result]).parent_path().parent_path().string();
    fprintf(stdout, "%s\n", InstallPrefix.c_str());
  }

  if (Options.is_set_by_user("current_rootfs")) {
    int ServerFD = FEXServerClient::ConnectToServer();
    if (ServerFD != -1) {
      auto RootFS = FEXServerClient::RequestRootFSPath(ServerFD);
      if (!RootFS.empty()) {
        fprintf(stdout, "%s\n", RootFS.c_str());
      }
    }
  }

  if (Options.is_set_by_user("tso_emulation_info")) {
    auto TSOFacts = GetTSOEmulationFacts();
    const char* GPRMemoryTSOEmulation {};
    const char* MemcpyMemoryTSOEmulation {};
    const char* VectorMemoryTSOEmulation {};
    const char* UnalignedMemoryLoadStoreTSOEmulation {};

    if (TSOFacts.HardwareTSO) {
      GPRMemoryTSOEmulation = "\e[32mHardware TSO\e[0m";
    } else if (TSOFacts.LRCPC3) {
      GPRMemoryTSOEmulation = "\e[32mLRCPC3\e[0m";
    } else if (TSOFacts.LRCPC2) {
      GPRMemoryTSOEmulation = "\e[32mLRCPC2\e[0m";
    } else if (TSOFacts.LRCPC1) {
      GPRMemoryTSOEmulation = "\e[32mLRCPC\e[0m";
    } else {
      GPRMemoryTSOEmulation = "\e[31mAtomics\e[0m";
    }

    // Memcpy only uses Hardware TSO, LRCPC, and Atomics.
    if (TSOFacts.HardwareTSO) {
      MemcpyMemoryTSOEmulation = "\e[32mHardware TSO\e[0m";
    } else if (TSOFacts.LRCPC1) {
      MemcpyMemoryTSOEmulation = "\e[32mLRCPC\e[0m";
    } else {
      MemcpyMemoryTSOEmulation = "\e[31mAtomics\e[0m";
    }

    if (TSOFacts.HardwareTSO) {
      VectorMemoryTSOEmulation = "\e[32mHardware TSO\e[0m";
    } else if (TSOFacts.LRCPC3) {
      VectorMemoryTSOEmulation = "\e[32mLRCPC3\e[0m";
    } else {
      VectorMemoryTSOEmulation = "\e[31mHalf-Barriers\e[0m";
    }

    if (TSOFacts.HardwareTSO) {
      UnalignedMemoryLoadStoreTSOEmulation = "\e[32mHardware TSO\e[0m";
    } else {
      UnalignedMemoryLoadStoreTSOEmulation = "\e[31mHalf-Barriers\e[0m";
    }

    fprintf(stdout, "Hardware Features:\n");
    fprintf(stdout, "\tMemory atomics emulation method:      %s\n", TSOFacts.LSE ? "\e[32mLSE\e[0m" : "\e[31mLL/SC\e[0m");
    fprintf(stdout, "\tUnaligned atomic memory granularity:  %s\n", TSOFacts.LSE2 ? "\e[32m16-byte\e[0m" : "\e[31mNatural alignment\e[0m");
    ///< TODO: Once TME is supported by hardware this can change.
    fprintf(stdout, "\tUnaligned memory loadstore emulation: %s\n", UnalignedMemoryLoadStoreTSOEmulation);
    fprintf(stdout, "\t16-Byte split-lock atomic emulation:  %s\n", TSOFacts.LSE ? "\e[31mTearing CAS loops\e[0m" : "\e[31mTearing LL/SC loops\e[0m");
    fprintf(stdout, "\t64-Byte split-lock atomic emulation:  %s\n", TSOFacts.LSE ? "\e[31mTearing CAS loops\e[0m" : "\e[31mTearing LL/SC loops\e[0m");
    fprintf(stdout, "\tGPR memory model emulation:           %s\n", GPRMemoryTSOEmulation);
    fprintf(stdout, "\tMemcpy memory model emulation:        %s\n", MemcpyMemoryTSOEmulation);
    fprintf(stdout, "\tVector memory model emulation:        %s\n", VectorMemoryTSOEmulation);

    FEX_CONFIG_OPT(TSOEnabled, TSOENABLED);
    FEX_CONFIG_OPT(MemcpySetTSOEnabled, MEMCPYSETTSOENABLED);
    FEX_CONFIG_OPT(VectorTSOEnabled, VECTORTSOENABLED);
    FEX_CONFIG_OPT(HalfBarrierTSOEnabled, HALFBARRIERTSOENABLED);
    FEX_CONFIG_OPT(StrictInProcessSplitLocks, STRICTINPROCESSSPLITLOCKS);
    fprintf(stderr, "Strict: %d\n", StrictInProcessSplitLocks());

    fprintf(stdout, "\nConfiguration:\n");
    fprintf(stdout, "\tTSO Emulation:                        %s\n", TSOEnabled() ? "Enabled" : "Disabled");
    fprintf(stdout, "\tMemcpy TSO Emulation:                 %s\n", TSOEnabled() && MemcpySetTSOEnabled() ? "Enabled" : "Disabled");
    fprintf(stdout, "\tVector TSO Emulation:                 %s\n", TSOEnabled() && VectorTSOEnabled() ? "Enabled" : "Disabled");
    fprintf(stdout, "\tHalf-barrier unaligned TSO emulation: %s\n", TSOEnabled() && HalfBarrierTSOEnabled() ? "Enabled" : "Disabled");
    fprintf(stdout, "\t16-Byte strict split-lock emulation:  %s\n", StrictInProcessSplitLocks() ? "In-process mutex" : "Tearing");
    fprintf(stdout, "\t64-Byte strict split-lock emulation:  %s\n", StrictInProcessSplitLocks() ? "In-process mutex" : "Tearing");
  }

  return 0;
}
