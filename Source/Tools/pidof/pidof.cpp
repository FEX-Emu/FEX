// SPDX-License-Identifier: MIT
#include "OptionParser.h"

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

    auto IsFEX = [](auto& Path) {
      if (Path.ends_with("FEXInterpreter")) {
        return true;
      }
      if (Path.ends_with("FEXLoader")) {
        return true;
      }

      return false;
    };
    bool IsFEXBin = IsFEX(pid.exe_link) || IsFEX(Args[0]);
    if (!IsFEXBin) {
      continue;
    }

    auto Arg1 = Args[1];
    auto Arg1Program = std::filesystem::path(Arg1).filename();

    bool Matched = false;
    for (const auto& CompareProgram : Config::Programs) {
      auto CompareProgramFilename = std::filesystem::path(CompareProgram).filename();
      if (CompareProgram == Arg1Program || CompareProgram == Arg1 || CompareProgramFilename == Arg1Program) {
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
