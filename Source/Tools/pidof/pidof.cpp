// SPDX-License-Identifier: MIT
#include "OptionParser.h"

#include <FEXHeaderUtils/Filesystem.h>

#include <charconv>
#include <cstring>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <string>
#include <unordered_set>

namespace Config {

bool SingleShot {};
bool SkipZombie {true};
bool DoNotDisplay {};
std::string Separator {" "};
std::unordered_set<int64_t> OmitPids;
std::unordered_set<std::string> Programs;

void LoadOptions(int argc, char** argv) {
  optparse::OptionParser Parser {};

  Parser.add_option("-s").help("Single shot - Only returns one pid").action("store_true").set_default(SingleShot);

  Parser.add_option("-q")
    .help("Do not display matched PIDs to stdout. Simply exit with status of true or false if a PID was found")
    .action("store_true")
    .set_default(DoNotDisplay);

  Parser.add_option("-z").help("Try to detect zombie processes - Usually zombie processes are skipped").action("store_false").set_default(SkipZombie);

  Parser.add_option("-d").help("Use a different separator if more than one pid is show - Default is space").set_default(Separator);

  Parser.add_option("-o").help("Ignore processes with matched pids").action("append");

  optparse::Values Options = Parser.parse_args(argc, argv);

  SingleShot = Options.get("s");
  DoNotDisplay = Options.get("q");
  SkipZombie = Options.get("z");
  Separator = Options["d"];

  for (const auto& Omit : Options.all("o")) {
    std::istringstream ss {Omit};
    std::string sub;
    while (std::getline(ss, sub, ',')) {
      int64_t pid;
      auto ConvResult = std::from_chars(sub.data(), sub.data() + sub.size(), pid, 10);

      // Invalid pid, skip.
      if (ConvResult.ec == std::errc::invalid_argument) {
        continue;
      }

      OmitPids.emplace(pid);
    }
  }

  for (const auto& Program : Parser.args()) {
    Programs.emplace(Program);
  }
}
} // namespace Config

bool FindWineFEXApplication(int64_t PID, std::string_view exe, const std::vector<std::string_view>& Args) {
  // Walk the arguments and see if anything contains wine.
  bool FoundWine = false;

  if (exe.find("wine") != exe.npos) {
    FoundWine = true;
  }

  if (!FoundWine) {
    for (auto Arg : Args) {
      if (Arg.find("wine") != Arg.npos) {
        FoundWine = true;
        break;
      }
    }
  }

  if (!FoundWine) {
    return false;
  }

  // Wine was found, scan the mapped files to see if anything mapped "libarm64ecfex.dll" or "libwow64fex.dll"
  for (const auto& Entry : std::filesystem::directory_iterator(fmt::format("/proc/{}/map_files", PID))) {
    // If not a symlink then skip.
    if (!Entry.is_symlink()) {
      continue;
    }

    const auto filename = std::filesystem::read_symlink(Entry.path()).filename().string();
    if (filename.find("arm64ecfex.dll") != filename.npos || filename.find("wow64fex.dll") != filename.npos) {
      return true;
    }
  }

  return false;
}

struct PIDInfo {
  int64_t pid;
  std::string cmdline;
  std::string exe_link;
  char State;
};

std::vector<PIDInfo> PIDs;

int main(int argc, char** argv) {
  Config::LoadOptions(argc, argv);

  // Iterate over all pids, storing the data for investigating afterwards.
  for (const auto& Entry : std::filesystem::directory_iterator("/proc/")) {
    // If not a directory then skip.
    if (!Entry.is_directory()) {
      continue;
    }

    auto CMDLinePath = Entry.path() / "cmdline";
    auto StatusPath = Entry.path() / "status";
    auto ExePath = Entry.path() / "exe";

    // If cmdline doesn't exist then skip.
    if (!std::filesystem::exists(CMDLinePath)) {
      continue;
    }

    auto Filename = Entry.path().filename().string();
    int64_t pid;
    auto ConvResult = std::from_chars(Filename.data(), Filename.data() + Filename.size(), pid, 10);

    // If the filename couldn't be converted to a PID then skip.
    // Happens with folders like `self` and a few other folders in this directory.
    if (ConvResult.ec == std::errc::invalid_argument) {
      continue;
    }

    std::ostringstream CMDLineData;
    {
      std::ifstream fs(CMDLinePath, std::ios_base::in | std::ios_base::binary);

      if (!fs.is_open()) {
        continue;
      }

      CMDLineData << fs.rdbuf();

      // If cmdline was empty then skip.
      if (CMDLineData.str().empty()) {
        continue;
      }
    }

    std::error_code ec;
    std::string exe_link = std::filesystem::read_symlink(ExePath, ec);

    // Couldn't read exe path? skip.
    if (ec) {
      continue;
    }

    // Read state
    char State {};

    {
      std::ifstream fs(StatusPath, std::ios_base::in | std::ios_base::binary);

      if (!fs.is_open()) {
        continue;
      }

      std::string Line;

      while (std::getline(fs, Line)) {
        if (fs.eof()) {
          break;
        }

        if (Line.find("State") == Line.npos) {
          continue;
        }

        if (sscanf(Line.c_str(), "State: %c", &State) == 1) {
          break;
        }
      }
    }

    PIDs.emplace_back(PIDInfo {
      .pid = pid,
      .cmdline = CMDLineData.str(),
      .exe_link = std::move(exe_link),
      .State = State,
    });
  }

  std::unordered_set<int64_t> MatchedPIDs;
  for (const auto& pid : PIDs) {
    if (pid.State == 'Z' && Config::SkipZombie) {
      continue;
    }
    if (Config::OmitPids.contains(pid.pid)) {
      continue;
    }

    std::vector<std::string_view> Args;
    const char* arg = pid.cmdline.data();

    while (arg[0]) {
      Args.emplace_back(arg);
      arg += strlen(arg) + 1;
    }

    auto FindFEXArgument = [](auto& Path) -> int32_t {
      if (Path.ends_with("FEXInterpreter")) {
        return 1;
      }
      if (Path.ends_with("FEXLoader")) {
        return 1;
      }

      return -1;
    };

    struct ProgramPair {
      std::string_view ProgramPath;
      std::string_view ProgramFilename;
    };

    auto FindEmulatedWineArgument = [](int32_t BeginningArg, const std::vector<std::string_view>& Args, bool Wine) -> ProgramPair {
      std::string_view ProgramName = Args[BeginningArg];

      for (size_t i = BeginningArg; i < Args.size(); ++i) {
        auto CurrentProgramName = FHU::Filesystem::GetFilename(Args[i]);

        if (CurrentProgramName == "wine-preloader" || CurrentProgramName == "wine64-preloader") {
          // Wine preloader is required to be in the format of `wine-preloader <wine executable>`
          // The preloader doesn't execve the executable, instead maps it directly itself
          // Skip the next argument since we know it is wine (potentially with custom wine executable name)
          ++i;
          Wine = true;
        } else if (CurrentProgramName == "wine" || CurrentProgramName == "wine64") {
          // Next argument, this isn't the program we want
          //
          // If we are running wine or wine64 then we should check the next argument for the application name instead.
          // wine will change the active program name with `setprogname` or `prctl(PR_SET_NAME`.
          // Since FEX needs this data far earlier than libraries we need a different check.
          Wine = true;
        } else {
          if (Wine == true) {
            // If this was path separated with '\' then we need to check that.
            auto WinSeparator = CurrentProgramName.find_last_of('\\');
            if (WinSeparator != CurrentProgramName.npos) {
              // Used windows separators
              CurrentProgramName = CurrentProgramName.substr(WinSeparator + 1);
            }

            return {
              .ProgramPath = Args[i],
              .ProgramFilename = CurrentProgramName,
            };
          }
          break;
        }
      }

      auto ProgramFilename = ProgramName;
      auto Separator = ProgramName.find_last_of('/');
      if (Separator != ProgramName.npos) {
        // Used windows separators
        ProgramFilename = ProgramFilename.substr(Separator + 1);
      }

      return {
        .ProgramPath = ProgramName,
        .ProgramFilename = ProgramFilename,
      };
    };

    int32_t ProgramArg = -1;
    ProgramArg = FindFEXArgument(pid.exe_link);
    if (ProgramArg == -1) {
      ProgramArg = FindFEXArgument(Args[0]);
    }

    bool IsWine = false;
    if (ProgramArg == -1) {
      // If we still haven't found a FEXInterpreter path then this might be an arm64ec FEX application.
      // The only way to know for sure is the walk the mapped files of the process and check if FEX is mapped.
      if (FindWineFEXApplication(pid.pid, pid.exe_link, Args)) {
        // Search from the start.
        ProgramArg = 0;
        IsWine = true;
      }
    }

    if (ProgramArg == -1 || ProgramArg >= Args.size()) {
      continue;
    }

    ProgramPair Arg = FindEmulatedWineArgument(ProgramArg, Args, IsWine);
    bool Matched = false;
    for (const auto& CompareProgram : Config::Programs) {
      auto CompareProgramFilename = std::filesystem::path(CompareProgram).filename();
      if (CompareProgram == Arg.ProgramFilename || CompareProgram == Arg.ProgramPath || CompareProgramFilename == Arg.ProgramFilename) {
        MatchedPIDs.emplace(pid.pid);
        Matched = true;
        break;
      }
    }

    if (Matched && Config::SingleShot) {
      break;
    }
  }

  if (!MatchedPIDs.empty() && !Config::DoNotDisplay) {
    bool first = true;
    for (const auto& MatchedPID : MatchedPIDs) {
      if (first) {
        fmt::print("{}", MatchedPID);
        first = false;
      } else {
        fmt::print("{}{}", Config::Separator, MatchedPID);
      }
    }
    fmt::print("\n");
  }

  return MatchedPIDs.empty() ? 1 : 0;
}
